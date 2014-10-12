#include "htmlparser.hpp"

#if defined _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#endif

static std::string cleantext(GumboNode* node) {
  if (node->type == GUMBO_NODE_TEXT) {
    return std::string(node->v.text.text);
  } else if (node->type == GUMBO_NODE_ELEMENT &&
             node->v.element.tag != GUMBO_TAG_SCRIPT &&
             node->v.element.tag != GUMBO_TAG_STYLE) {
    std::string contents = "";
    GumboVector* children = &node->v.element.children;
    for (int i = 0; i < children->length; ++i) {
      const std::string text = cleantext((GumboNode*) children->data[i]);
      if (i != 0 && !text.empty()) {
        contents.append(" ");
      }
      contents.append(text);
    }
    return contents;
  } else {
    return "";
  }
}

static void search_for_links(GumboNode* node, std::vector <link_info> *outvec ) {
  if (node->type != GUMBO_NODE_ELEMENT) {
    return;
  }
  GumboAttribute* href;
  if (node->v.element.tag == GUMBO_TAG_A &&
      (href = gumbo_get_attribute(&node->v.element.attributes, "href"))) {
    link_info a;
    a.linkText = cleantext(node);
    a.linkURI = href->value;

    outvec->push_back(a);
  }

  GumboVector* children = &node->v.element.children;
  for (int i = 0; i < children->length; ++i) {
    search_for_links(static_cast<GumboNode*>(children->data[i]), outvec);
  }
}

static const char* find_title(const GumboNode* root)
{
    if(root && root->type == GUMBO_NODE_ELEMENT && root->v.element.children.length >= 2)
    {
        const GumboVector* root_children = &root->v.element.children;
        GumboNode* head = NULL;

        for (long i = 0; i < root_children->length; i++)
        {
            GumboNode* child = (GumboNode*)root_children->data[i];
            if(child  && child->type == GUMBO_NODE_ELEMENT && child->v.element.tag == GUMBO_TAG_HEAD)
            {
                head = child;
                break;
            }
        }
        if(head)
        {
            GumboVector* head_children = &head->v.element.children;
            for (long i = 0; i < head_children->length; i++)
            {
                GumboNode* child = (GumboNode*)head_children->data[i];
                if (child && child->type == GUMBO_NODE_ELEMENT && child->v.element.tag == GUMBO_TAG_TITLE)
                {
                    if (child->v.element.children.length < 1)
                    {
                        return "";
                    }
                    GumboNode* title_text = (GumboNode*)child->v.element.children.data[0];

                    if(title_text && title_text->type == GUMBO_NODE_TEXT)
                    {
                        return title_text->v.text.text;
                    }
                }
            }
        }
    }

    return "";
}

static int TextAndLinksWithPos(GumboNode* node, std::vector <extendedLinkInfo> &a, std::string &txt, bool flag = false)
{
	if (node->type == GUMBO_NODE_TEXT)
	{
		std::string s = node->v.text.text;

		if (flag && s.length())
		{
			txt.append(" ");
		}
		txt.append(s);
		return 0;
	}
	else if (node->type == GUMBO_NODE_ELEMENT
		&& node->v.element.tag != GUMBO_TAG_SCRIPT
		&& node->v.element.tag != GUMBO_TAG_STYLE)
	{
		GumboAttribute* href = NULL;
		extendedLinkInfo eli;
		if (node->v.element.tag == GUMBO_TAG_A)
		{
			if ((href = gumbo_get_attribute(&node->v.element.attributes, "href")))
			{
				eli.href = href->value;
				eli.textStartPos = txt.length();
			}
		}
		GumboVector* children = &node->v.element.children;
		for (long i = 0; i < children->length; i++)
		{
			TextAndLinksWithPos((GumboNode*)children->data[i], a, txt, true);
		}

		if (node->v.element.tag == GUMBO_TAG_A && href)
		{
			eli.textEndPos = txt.length();
			eli.text = txt.substr(eli.textStartPos, eli.textEndPos);
			a.push_back(eli);
		}
	}
	return 0;
}

static int ParseMetaTags(GumboNode* node, std::vector <MetaInfo> &a)
{
	if (node->type == GUMBO_NODE_ELEMENT&& node->v.element.tag != GUMBO_TAG_SCRIPT && node->v.element.tag != GUMBO_TAG_STYLE)
	{
		if (node->v.element.tag == GUMBO_TAG_META)
		{
			MetaInfo mi;
			GumboAttribute* M_name = NULL;
			GumboAttribute* M_content = NULL;

			if ((M_name = gumbo_get_attribute(&node->v.element.attributes, "name")))
			{
				mi.name = M_name->value;
			}

			if ((M_content = gumbo_get_attribute(&node->v.element.attributes, "content")))
			{
				mi.content = M_content->value;
			}

			a.push_back(mi);
		}

		GumboVector* children = &node->v.element.children;
		for (long i = 0; i < children->length; i++)
		{
			ParseMetaTags((GumboNode*)children->data[i], a);
		}
	}
	return 0;
}

HTMLParser::HTMLParser()
{
    this->raw = "";
    this->curlclass = new CURLClass();
    this->curlclass->SetGetHeader(true);
    this->parsed = false;
    this->DOM = NULL;
}

HTMLParser::~HTMLParser()
{
    if(this->curlclass != NULL)
    {
	delete this->curlclass;
	this->curlclass = NULL;
    }	
    if(this->DOM != NULL)
    {
        gumbo_destroy_output(&kGumboDefaultOptions, this->DOM);
    }
}

int HTMLParser::FetchURLContents(std::string url, long max_len)
{
    this->raw = this->curlclass->FetchURLContents(url, max_len);
    this->url = this->curlclass->GetEffectiveURL();
    this->parsed = false;
    return 0;
}

std::string HTMLParser::GetRawResponse()
{
    return this->raw;
}

int HTMLParser::ParseResponse()
{
    if(!this->raw.length())
        return 0;

	this->FindCharset();

    std::vector <std::string> exp;

    long lines = explode("\n", this->raw, exp);
    std::string comp = "";

    this->headers = "";

    boost::regex http_exp("^(HTTP|http)/([0-9]{1}\\.?[0-9]?)( +)([0-9]{1,3})(.*)$");
    boost::cmatch http_exp_match;

    long i;
    for(i = 0; i < lines; i++)
    {
        //std::cout << exp[i] << std::endl;

        if(trim(exp[i]) == comp)
        {
            //std::cout << "Breaking on line : " << i << std::endl;

            if(i+1 < lines && trim(exp[i+1]) == comp)
            {
                if(i > 0)
                {
                    this->headers.append("\n");
                }
                this->headers.append(exp[i]);
            }
			else if (i + 1 < lines && boost::regex_match(exp[i + 1].c_str(), http_exp_match, http_exp))
            {
                if(i > 0)
                {
                    this->headers.append("\n");
                }
                this->headers.append(exp[i]);
            }
            else
            {
                break;
            }

        }
        else
        {
            if(i > 0)
            {
                this->headers.append("\n");
            }
            this->headers.append(exp[i]);

        }
    }

    this->contents = "";
    for(; i < lines; i++)
    {
        this->contents.append(exp[i]);
        this->contents.append("\n");
    }

    //std::cout << "Final headers : " << std::endl;
    //std::cout << this->headers << std::endl;
	if (this->charset.length())
	{
		std::string temp = "";
		this->Convert(this->charset, "UTF-8", this->contents, temp);

		if (temp.length())
		{
			this->contents = temp;
		}
	}
	

    return 0;
}


int HTMLParser::ParseDOM()
{
    if(this->DOM != NULL)
    {
        gumbo_destroy_output(&kGumboDefaultOptions, this->DOM);
    }

    if(this->contents.length())
    {
#if defined _MSC_VER
		for(long i = 0; i < this->contents.length(); i++)
		{
			if (this->contents[i] < 0)
			{
				this->contents[i] = (char)((int)this->contents[i] % 128);

				if (this->contents[i] < 0)
					this->contents[i] = (char)((int)this->contents[i] + 128);
			}
			else if (this->contents[i] > 128)
			{
				this->contents[i] = (char)(this->contents[i] % 128);
			}
		}
#endif
        this->DOM = gumbo_parse(this->contents.c_str());
    }
    else
    {
        this->DOM = NULL;
    }
    return 0;
}

GumboOutput *HTMLParser::GetDOM()
{
    return this->DOM;
}

std::string HTMLParser::GetHeaders()
{
    return this->headers;
}

int HTMLParser::ParseHeaders()
{
    if(this->headers.length())
    {
        std::vector<std::string> exp;
        explode("\n", this->headers, exp);

        boost::regex http_exp("^(HTTP|http)/([0-9]{1}\\.?[0-9]?)( +)([0-9]{1,3})(.*)$");
        boost::cmatch http_exp_match;
        bool http_exp_found = false;

        for(long i = 0; i < exp.size(); i++)
        {
            if(!http_exp_found)
            {
                //std::cout << "Checking : " << exp[i] << std::endl;
                if(boost::regex_match(exp[i].c_str(), http_exp_match, http_exp))
                {
                    http_exp_found = true;
                    //std::cout << "Match found : " << std::endl;
                    //std::cout << http_exp_match << std::endl;

                    //std::cout << http_exp_match[2] << std::endl;
                    this->HTTPVersion = boost::lexical_cast<float>(http_exp_match[2]);

                    //std::cout << http_exp_match[4] << std::endl;
                    this->HTTPResponseCode = boost::lexical_cast<int> (http_exp_match[4]);

                    //std::cout << http_exp_match[5] << std::endl;
                    this->HTTPResponseMessage = http_exp_match[5];

                    continue; // we don't want to add the protocol line to the map
                }
            }

            std::size_t pos = exp[i].find(':');

            if(pos != std::string::npos)
            {
                std::string key = exp[i].substr(0, pos);

                std::string val = (pos<exp[i].length()?exp[i].substr(pos+1, std::string::npos):"");

                //std::cout << " Key : " << key << std::endl << " Value : " << val << std::endl;

                this->p_headers.insert(std::pair<std::string, std::string>(key, val));
            }

        }
        this->header_parsed = true;
    }
	return 0;
}

int HTMLParser::GetResponseCode()
{
    return this->HTTPResponseCode; 
}

std::string HTMLParser::GetHTML()
{
    return this->contents;
}

std::string HTMLParser::GetTitle()
{
    std::string ret = "";
    if(this->DOM)
    {
         ret = find_title(this->DOM->root);
    }
    return ret;
}

std::string HTMLParser::GetCleanText()
{
    std::string ret = "";
    if(this->DOM)
    {
        ret = cleantext(this->DOM->root);
    }
    return ret;
}

int HTMLParser::GetLinks(std::vector <link_info> &a)
{
	int ret = 0;
    if(this->DOM)
    {
        std::vector <link_info> b;
        search_for_links(this->DOM->root, &b);

        if(b.size())
        {
            //std::cout << "THIS URL:" << this->url << std::endl;
            URIParse u(this->url);
			//std::cout <<" Parsed url " << std::endl;
            for(long i = 0; i < b.size(); i++)
            {
                link_info a_inf;
                URIParse chk(b[i].linkURI);

                a_inf.linkText = trim(b[i].linkText);
                a_inf.linkURI = (chk.isValid()?chk.ReconstructURL():u.MakeAbsoluteURI(b[i].linkURI));
                a.push_back(a_inf);
            }
			ret = b.size();
            b.clear();
        }
    }

    return ret;
}

int HTMLParser::GetLinksAndText(std::vector <extendedLinkInfo> &a, std::string &txt)
{
	int ret = 0;

	if (this->DOM)
	{
		std::vector <extendedLinkInfo> b;
		//std::string t;
		TextAndLinksWithPos(this->DOM->root, b, txt, false);
		URIParse u(this->url);

		for (long i = 0; i < b.size(); i++)
		{
			extendedLinkInfo linf;
			URIParse chk(b[i].href);

			linf.text = trim(b[i].text);
			linf.textStartPos = b[i].textStartPos;
			linf.textEndPos = b[i].textEndPos;

			linf.href = (chk.isValid() ? chk.ReconstructURL() : u.MakeAbsoluteURI(b[i].href));

			a.push_back(linf);
		}

		ret = b.size();
		b.clear();
	}
	return ret;
}

int HTMLParser::FindCharset()
{
	boost::regex p1("charset[\\s]*=[\\s]*([a-z0-9_-]+)", boost::regex::icase);
	boost::regex p2("encoding[\\s]*=[\\s]*([W]{1})([a-z0-9_-]+)", boost::regex::icase);
	boost::regex p3("<[\\s]*meta[\\s]+charset[\\s]*=[\\s]*(\'|\")?([a-z0-9_-]+)(\'|\")?[\\s]*>", boost::regex::icase);
	boost::regex p4("<[\\s]*meta[\\s]+http-equiv[\\s]*=[\\s]*(\'|\")?[\\s]*content-type[\\s]*(\'|\")?[\\s]*content[\\s]*=[\\s]*(\'|\")?[\\s]*text\\/html[\\s]*;[\\s]*charset[\\s]*=[\\s]*([a-z0-9_-]+)[\\s]*(\'|\")?[\\s]*>", boost::regex::icase);
	//"<meta http-equiv=\"content-type\" content=\"text/html; charset=iso-8859-1\">"
	std::string::const_iterator start, end;
	boost::match_results<std::string::const_iterator> what;
	boost::match_flag_type flags;

	start = this->raw.begin();
	end = this->raw.end();
	flags = boost::match_default;

	std::vector <std::string> found;

	while (regex_search(start, end, what, p1, flags))
	{
		std::string fnd = what[1];
		found.push_back(fnd);
		start = what[0].second;
		// update flags:
		flags |= boost::match_prev_avail;
		flags |= boost::match_not_bob;
	}

	start = this->raw.begin();
	end = this->raw.end();
	flags = boost::match_default;
	while (regex_search(start, end, what, p2, flags))
	{
		std::string fnd = what[2];
		found.push_back(fnd);
		start = what[0].second;
		// update flags:
		flags |= boost::match_prev_avail;
		flags |= boost::match_not_bob;
	}

	start = this->raw.begin();
	end = this->raw.end();
	flags = boost::match_default;
	while (regex_search(start, end, what, p3, flags))
	{
		std::string fnd = what[2];
		found.push_back(fnd);
		start = what[0].second;
		// update flags:
		flags |= boost::match_prev_avail;
		flags |= boost::match_not_bob;
	}

	start = this->raw.begin();
	end = this->raw.end();
	flags = boost::match_default;
	while (regex_search(start, end, what, p4, flags))
	{
		std::string fnd = what[4];
		found.push_back(fnd);
		start = what[0].second;
		// update flags:
		flags |= boost::match_prev_avail;
		flags |= boost::match_not_bob;
	}

	if (found.size())
	{
		this->charset.append(found[found.size() - 1]);
		found.clear();
	}

	return 0;
}


int HTMLParser::Convert(std::string from, std::string to, std::string &input, std::string &output)
{
	size_t input_size = input.length()+1;
	size_t output_buf_size = 2 * input_size;

	char *input_cpy = (char*)calloc(sizeof(char), input_size+1);//new char[input_size];
	char *output_buf = (char*)calloc(sizeof(char), output_buf_size);//new char[output_buf_size];
	char *wrt_ptr = output_buf;
	char *inp_ptr = input_cpy;

#if defined _MSC_VER
	strcpy_s(input_cpy, input_size+1, input.c_str());
#else
	strcpy(input_cpy, input.c_str());
#endif


	iconv_t cd;
	if(from == "gb2312" || from == "GB2312")
	{
		from = "GB18030";
	}

	if ((cd = iconv_open(to.c_str(), from.c_str())) == (iconv_t)(-1))
	{
		std::cerr << "Cannot open converter from " << from << " to " << to << std::endl;
		return -1;
	}
	//std::cout << "Input text size : " << input_size << std::endl;
#if defined _MSC_VER
	int rc = iconv(cd, (const char**)&inp_ptr, &input_size, &wrt_ptr, &output_buf_size);
#else
	int rc = iconv(cd, &inp_ptr, &input_size, &wrt_ptr, &output_buf_size);
#endif

	if (rc == -1)
	{
		iconv_close(cd);
		std::cerr << "Error in converting from "<< from << " to " << to << std::endl;
		delete input_cpy;
		delete output_buf;
		return -1;
	}
	*((char*)wrt_ptr) = (char)0;
	//std::cout << "Output size: " << output_buf_size << std::endl;

	//for(size_t i = 0; i < output_buf_size; i++)
	//{
	//    std::cout<<"["<<(int)output_buf[i]<<"]";
	//}
	//std::cout << std::endl;
	//std::cout << output_buf << std::endl;
	output.append(output_buf);
	iconv_close(cd);
	//delete input_cpy;
	//delete output_buf;
	free(input_cpy);
	free(output_buf);
	return 0;
}

int HTMLParser::GetMetaTags(std::vector <MetaInfo> &a)
{
	a.clear();
	if (this->DOM)
	{
		ParseMetaTags(this->DOM->root, a);
	}
	return 0;
}
