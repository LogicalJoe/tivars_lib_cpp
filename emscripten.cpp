/*
 * Part of tivars_lib_cpp
 * (C) 2015-2017 Adrien 'Adriweb' Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#ifdef __EMSCRIPTEN__

#include "src/TIModels.h"
#include "src/TIVarTypes.h"
#include "src/TypeHandlers/TypeHandlers.h"

#include <emscripten.h>

using namespace tivars;

extern "C" int EMSCRIPTEN_KEEPALIVE main(int, char**)
{
    TIModels::initTIModelsArray();
    TIVarTypes::initTIVarTypesArray();
    TH_0x05::initTokens();

    puts("tivars_lib ready!");

    return 0;
}

#endif