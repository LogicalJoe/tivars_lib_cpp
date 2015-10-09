/*
 * Part of tivars_lib_cpp
 * (C) 2015 Adrien 'Adriweb' Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#include "TH_0x05.h"
#include "../utils.h"
#include <fstream>
#include <regex>
#include <sstream>

using namespace std;

namespace tivars
{

    std::unordered_map<uint, std::vector<std::string>> tokens_BytesToName;
    std::unordered_map<std::string, uint> tokens_NameToBytes;
    uint lengthOfLongestTokenName;
    std::vector<uint> firstByteOfTwoByteTokens;

    data_t TH_0x05::makeDataFromString(const string& str, const options_t options)
    {
        vector<uint> data;

        // two bytes reserved for the size. Filled later
        data.push_back(0); data.push_back(0);

        for (uint strCursorPos = 0; strCursorPos < str.length(); strCursorPos++)
        {
            for (uint currentLength = lengthOfLongestTokenName; currentLength > 0; currentLength--)
            {
                string currentSubString = str.substr(strCursorPos, currentLength);
                if (is_in_umap_string_uint(tokens_NameToBytes, currentSubString))
                {
                    uint tokenValue = tokens_NameToBytes[currentSubString];
                    if (tokenValue > 0xFF)
                    {
                        data.push_back(tokenValue >> 8);
                    }
                    data.push_back(tokenValue & 0xFF);
                    strCursorPos += currentLength - 1;
                    break;
                }
            }
        }
        uint actualDataLen = (uint) (data.size() - 2);
        data[0] = actualDataLen & 0xFF;
        data[1] = (actualDataLen >> 8) & 0xFF;
        return data;
    }

    string TH_0x05::makeStringFromData(const data_t& data, const options_t options)
    {
        if (data.size() < 2)
        {
            throw invalid_argument("Empty data array. Needs to contain at least 2 bytes (size fields)");
        }

        uint langIdx = (has_option(options, "lang") && options.at("lang") == LANG_FR) ? LANG_FR : LANG_EN;

        uint howManyBytes = (data[0] & 0xFF) + ((data[1] << 8) & 0xFF00);

        if (howManyBytes != data.size() - 2)
        {
            cerr << "[Warning] Token count (" << (data.size() - 2) << ") and size field (" << howManyBytes  << ") mismatch!";
        }

        uint errCount = 0;
        string str("");
        for (uint i = 2; i < howManyBytes + 2; i++)
        {
            uint currentToken = data[i];
            uint nextToken = (i < howManyBytes-1) ? data[i+1] : (uint)-1;
            uint bytesKey = currentToken;
            if (is_in_vector_uint(firstByteOfTwoByteTokens, currentToken))
            {
                if (nextToken == (uint)-1)
                {
                    cerr << "[Warning] Encountered an unfinished two-byte token! Setting the second byte to 0x00";
                    nextToken = 0x00;
                }
                bytesKey = nextToken + (currentToken << 8);
                i++;
            }
            if (tokens_BytesToName.find(bytesKey) != tokens_BytesToName.end())
            {
                str += tokens_BytesToName[bytesKey][langIdx];
            } else  {
                str += " [???] ";
                errCount++;
            }
        }

        if (errCount > 0)
        {
            cerr << "[Warning] " << errCount << " token(s) could not be detokenized (' [???] ' was used)!";
        }

        if (has_option(options, "prettify") && options.at("prettify") == 1)
        {
            str = regex_replace(str, regex("\\[?\\|?([a-z]+)\\]?"), "$1");
        }

        if (has_option(options, "reindent") && options.at("reindent") == 1)
        {
            str = reindentCodeString(str);
        }

        return str;
    }

    string TH_0x05::reindentCodeString(const string& str_orig)
    {
        string str(str_orig);

        regex eolRegex("\"[^→\"\\n]+[→\"\\n]|(\\:)");
        string output_text;
        sregex_token_iterator begin(str.begin(), str.end(), eolRegex, {-1, 0});
        sregex_token_iterator end;
        for_each(begin, end, [&](const string& m) { output_text += (m == ":") ? "\n" : m; });
        str = output_text;

        str = regex_replace(str, regex("([^\\s])(Del|Eff)Var "), "$1\n$2Var");

        vector<string> lines_tmp = explode(str, '\n');
        vector<pair<uint, string>> lines; // indent, text
        for (uint i=0; i<lines_tmp.size(); i++)
        {
            lines.push_back(make_pair(0, lines_tmp[i]));
        }

        vector<string> increaseIndentAfter = { "If", "For", "While", "Repeat" };
        uint nextIndent = 0;
        string oldFirstCommand = "", firstCommand = "";
        for (uint key=0; key<lines.size(); key++)
        {
            auto lineData = lines[key];
            oldFirstCommand = firstCommand;

            string trimmedLine = trim(lineData.second);
            char* trimmedLine_c = (char*) trimmedLine.c_str();
            firstCommand = strtok(trimmedLine_c, " ");
            firstCommand = trim(firstCommand);
            trimmedLine = string(trimmedLine_c);
            trimmedLine_c = (char*) trimmedLine.c_str();
            if (firstCommand == trimmedLine)
            {
                firstCommand = strtok(trimmedLine_c, "(");
                firstCommand = trim(firstCommand);
            }

            lines[key].first = nextIndent;

            if (is_in_vector_string(increaseIndentAfter, firstCommand))
            {
                nextIndent++;
            }
            if (lines[key].first > 0 && (firstCommand == "Then" || firstCommand == "Else" || firstCommand == "End"))
            {
                lines[key].first--;
            }
            if (nextIndent > 0 && (firstCommand == "End" || (oldFirstCommand == "If" && firstCommand != "Then")))
            {
                nextIndent--;
            }
        }

        str = "";
        for (const auto& line : lines)
        {
            str += str_repeat(" ", line.first * 4) + line.second + '\n';
        }

        return str;
    }

    void TH_0x05::initTokens()
    {
        ifstream t("/Users/adriweb/Documents/tivars_lib_cpp/src/TypeHandlers/programs_tokens.csv");
        string csvFileStr((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());

        if (csvFileStr.length() > 0)
        {
            vector<vector<string>> lines;
            ParseCSV(csvFileStr, lines);

            for (const auto& tokenInfo : lines)
            {
                uint bytes;
                if (tokenInfo[6] == "2")
                {
                    if (!is_in_vector_uint(firstByteOfTwoByteTokens, hexdec(tokenInfo[7])))
                    {
                        firstByteOfTwoByteTokens.push_back(hexdec(tokenInfo[7]));
                    }
                    bytes = hexdec(tokenInfo[8]) + (hexdec(tokenInfo[7]) << 8);
                } else {
                    bytes = hexdec(tokenInfo[7]);
                }
                tokens_BytesToName[bytes] = { tokenInfo[4], tokenInfo[5] }; // EN, FR
                tokens_NameToBytes[tokenInfo[4]] = bytes; // EN
                tokens_NameToBytes[tokenInfo[5]] = bytes; // FR
                uint maxLenName = (uint) max(tokenInfo[4].length(), tokenInfo[5].length());
                if (maxLenName > lengthOfLongestTokenName)
                {
                    lengthOfLongestTokenName = maxLenName;
                }
            }
        } else {
            throw runtime_error("Could not open the tokens csv file");
        }
    }

}