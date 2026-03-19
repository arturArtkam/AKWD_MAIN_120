#ifndef ALLDATA_H_INCLUDED
#define ALLDATA_H_INCLUDED

#include "struct_of_ak120.h"

class All_data
{
public:
    OS::TMutex mtx;
    DataStructW_t vault;
};

//    void save(Sd_disk* sd_card)
//    {
//        _datasrc[TIME]->fill_datastruct(&_vault);
//        _datasrc[SENS]->fill_datastruct(&_vault);
//
//        sd_card->write<sizeof(DataStructR_t)>(&_vault.Time);
//    }

#endif // ALLDATA_H_INCLUDED
