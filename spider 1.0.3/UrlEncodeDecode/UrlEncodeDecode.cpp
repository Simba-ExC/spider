#include "UrlEncodeDecode.h"

std::string url_decode(const std::string& encoded)
{
	std::string output;
	for (size_t i(0); i < encoded.length(); ++i) {
		if (encoded[i] == '%' && i+2 < encoded.length()) {
			std::stringstream ss;
			ss << std::hex << encoded.substr(++i, 2);
			++i;
			uint32_t uint(0);
			ss >> uint;
			output.push_back(static_cast<unsigned char>(uint));
		}
		else output.push_back(encoded[i]);
	}
	return output;
}

std::string url_encode(const std::string& decoded)
{
	std::ostringstream escaped;
	escaped.fill('0');
	escaped << std::hex;
	for (auto i = decoded.begin(); i != decoded.end(); ++i) {
		std::string::value_type c = (*i);
		// Keep alphanumeric and other accepted characters intact
		if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == ':' || c == '/') {
			escaped << c;
		}
		// Any other characters are percent-encoded
		else {
			escaped << std::uppercase;
			escaped << '%' << std::setw(2) << int((unsigned char)c);
			escaped << std::nouppercase;
		}
	}
	return escaped.str();
}
