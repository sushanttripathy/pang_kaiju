#ifndef _TRIM_HPP_
#define _TRIM_HPP_

#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>

// trim from start
static inline std::string &ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {

#if defined _MSC_VER 
	long len = s.length();
	for (long i = 0; i < len; i++)
	{
		if (s[i] < 0)
		{
			s[i] = (char)((int)s[i] % 128);
			//s[i] = (unsigned char)32;
			if (s[i] < 0)
				s[i] = (char)(128 + (int)s[i]);
			//std::cout << ((256 + (int)s[i])) << std::endl;
			//std::cout << (int)s[i] << std::endl;
			//std::cout << std::endl;
		}
	}
#endif
        return ltrim(rtrim(s));
}

#endif // _TRIM_HPP_
