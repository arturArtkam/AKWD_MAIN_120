#ifndef SD_H_INCLUDED
#define SD_H_INCLUDED

//#include "akwd.h"
#include "../sd_card/sun_disk16.h"

class Sd_disk : public sd_card_t
{
public:
    Sd_disk() :
        sd_card_t()
    {
    }

    ~Sd_disk() = default;

    void save([[gnu::unused]] void* src)
    {

    }
};

#endif
