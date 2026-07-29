#include "Text.h"

#include <cstdio>

namespace ArkFind
{
	namespace Text
	{
		std::string Fill(const std::string& templateText, const Placeholders& values)
		{
			std::string out;
			out.reserve(templateText.size() + 32);

			size_t pos = 0;
			while (pos < templateText.size())
			{
				const size_t open = templateText.find('{', pos);
				if (open == std::string::npos)
				{
					out.append(templateText, pos, std::string::npos);
					break;
				}

				const size_t close = templateText.find('}', open + 1);
				if (close == std::string::npos)
				{
					out.append(templateText, pos, std::string::npos);
					break;
				}

				out.append(templateText, pos, open - pos);

				const std::string key = templateText.substr(open + 1, close - open - 1);

				bool replaced = false;
				for (const auto& value : values)
				{
					if (value.first == key)
					{
						out += value.second;
						replaced = true;
						break;
					}
				}

				if (!replaced)
				{
					out.append(templateText, open, close - open + 1);
				}

				pos = close + 1;
			}

			return out;
		}

		std::string Number(double value, int decimals)
		{
			if (decimals < 0)
			{
				decimals = 0;
			}
			if (decimals > 9)
			{
				decimals = 9;
			}

			char buffer[64];
			std::snprintf(buffer, sizeof(buffer), "%.*f", decimals, value);
			return buffer;
		}
	}
}
