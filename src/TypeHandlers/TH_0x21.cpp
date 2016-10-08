/*
 * Part of tivars_lib_cpp
 * (C) 2015-2016 Adrien "Adriweb" Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#include "TypeHandlers.h"
#include "../utils.h"

using namespace std;

namespace tivars
{

    data_t TH_0x21::makeDataFromString(const string& str, const options_t& options)
    {
        (void)options;

        throw runtime_error("Unimplemented");

        if (str == "" || !is_numeric(str))
        {
            throw runtime_error("Invalid input string. Needs to be a valid Exact Real Pi Fraction");
        }
    }

    string TH_0x21::makeStringFromData(const data_t& data, const options_t& options)
    {
        (void)options;

        if (data.size() != TH_0x21::dataByteCount)
        {
            throw invalid_argument("Empty data array. Needs to contain " + to_string(TH_0x21::dataByteCount) + " bytes");
        }

        string coeff = TH_0x00::makeStringFromData(data);

        string str = ((coeff != "0") ? (dec2frac(atof(coeff.c_str())) + "*π") : "");

        // Improve final display
        str = regex_replace(str, regex("\\+1\\*"), "+");  str = regex_replace(str, regex("\\(1\\*"),  "(");
        str = regex_replace(str, regex("-1\\*"),   "-");  str = regex_replace(str, regex("\\(-1\\*"), "(-");
        str = regex_replace(str, regex("\\+-"),    "-");

        // Shouldn't happen - I don't believe the calc generate such files.
        if (str == "")
        {
            str = "0";
        }

        return str;
    }

}