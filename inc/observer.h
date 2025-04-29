#ifndef OBSERVER_H
#define OBSERVER_H

template<typename Iobserver, size_t MAX>
class Observable
{
protected:
    Iobserver* _observers[MAX]; /* observers */
    size_t _tail;

public:
    Observable() :
        _observers{nullptr},
        _tail(0)
    {
    }

    bool register_item(Iobserver* observer)
    {
        if (_tail < MAX)
        {
            _observers[_tail++] = observer;
            return true;
        }
        else
            return false;
    }

    size_t remove_item(Iobserver* observer)
    {
        size_t deleted = 0;
        for (size_t idx = 0; idx < MAX; idx++)
        {
            if (_observers[idx] == observer)
            {
                size_t sub = idx;
                while (sub < MAX - 1)
                {
                    _observers[sub] = _observers[sub + 1];
                    _observers[sub + 1] = nullptr;
                    sub++;
                }

                idx--;
                deleted++;
            }
        }

        return deleted;
    }
};

#endif /* OBSERVER_H */
