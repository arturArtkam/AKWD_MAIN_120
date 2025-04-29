#ifndef OP_TIME_H
#define OP_TIME_H

#include <mutex>
#include "../fram/vault_fram.h"
#include "../../../Lib/bkp_domain.h"

/* —четчик продолжительности рабочих циклов */
class Cycle_time_counter final
{
private:
    OS::TMutex _op_mutex;

    mutable bkp_mem_t<uint32_t> _bkp_idle;
    mutable bkp_mem_t<uint32_t> _bkp_work;

public:
    Cycle_time_counter(uint32_t& idle, uint32_t& work) :
        _op_mutex(),
        _bkp_idle(),
        _bkp_work()
    {
        _bkp_idle.bind_to_bkpmem(&idle);
        _bkp_work.bind_to_bkpmem(&work);
    }

    void inc_work()
    {
        std::lock_guard<OS::TMutex> lock(_op_mutex);
        volatile uint32_t tmp = _bkp_work.get();
        _bkp_work.set(++tmp);
    }

    void inc_idle()
    {
        std::lock_guard<OS::TMutex> lock(_op_mutex);
        volatile uint32_t tmp = _bkp_idle.get();
        _bkp_idle.set(++tmp);
    }

    bool reset()
    {
        if (_op_mutex.try_lock())
        {
            _bkp_idle.set(0);
            _bkp_work.set(0);
            _op_mutex.unlock();

            return true;
        }
        else
        {
            return false;
        }
    }

//    size_t serialize(bool (*fram_write)(void const* , uint32_t, uint32_t), size_t& offset) override
//    {
//        fram_write(_bkp_idle.c_ref(), offset, _bkp_idle.size());
//        offset += _bkp_idle.size();
//
//        fram_write(_bkp_work.c_ref(), offset, _bkp_work.size());
//        offset += _bkp_work.size();
//
//        return offset;
//    }
//
//    size_t deserialize(bool (*fram_read)(void*, uint32_t, uint32_t), size_t& offset) override
//    {
//        uint32_t tmp;
//
//        fram_read(&tmp, offset, _bkp_idle.size());
//        _bkp_idle.set(tmp);
//        offset += _bkp_idle.size();
//
//        fram_read(&tmp, offset, _bkp_work.size());
//        _bkp_work.set(tmp);
//        offset += _bkp_work.size();
//
//        return offset;
//    }
//
//    size_t serialized_size() const override
//    {
//        return _bkp_idle.size() + _bkp_work.size();
//    }
};

#endif /* OP_TIME_H */
