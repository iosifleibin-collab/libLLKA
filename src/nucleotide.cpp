/* vim: set sw=4 ts=4 sts=4 expandtab : */

#include <llka_nucleotide.h>
#include "nucleotide.hpp"

LLKA_Bool LLKA_CC LLKA_isNucleotideCompound(const char *compId)
{
    return LLKAInternal::isNucleotideCompound(compId);
}

LLKA_SugarPucker LLKA_CC LLKA_nameToSugarPucker(const char *name)
{
    auto it = LLKAInternal::NAME_TO_SUGAR_PUCKER_MAPPING.find(name);
    if (it != LLKAInternal::NAME_TO_SUGAR_PUCKER_MAPPING.cend())
        return it->second;
    return LLKA_INVALID_SUGAR_PUCKER;
}
