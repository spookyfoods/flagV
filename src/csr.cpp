#include "csr.h"

bool CsrFile::isAccessValid(uint16_t funct12, PrivLvl priv, bool willWrite) {
    auto RWMASK = 0b1100'0000'0000;
    auto PLMASK = 0b0011'0000'0000;
    if (willWrite && (((funct12 & RWMASK) >> 10) == 0b11))
        return false;
    if (static_cast<PrivLvl>((funct12 & PLMASK) >> 8) > priv)
        return false;
    return true;
}
