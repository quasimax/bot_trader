#ifndef TKEYBOARD_H
#define TKEYBOARD_H

#include <boost/property_tree/ptree.hpp>

namespace pt = boost::property_tree;

class TlgKeyboard
{
public:
    enum Side {Vertical, Horizontal};
    void add_keys(const std::vector<std::string>& keys, Side side = Vertical)
    {
        pt::ptree karray;
        pt::ptree key;

        switch(side)
        {
            case Horizontal:
                for(auto& kdata : keys)
                {
                    key.put("text", kdata);
                    karray.push_back(std::make_pair("", key));
                }
                m_keyboard.push_back(std::make_pair("", karray));
                karray.clear();
                break;

            case Vertical:
                for(auto& kdata : keys)
                {
                    key.put("text", kdata);
                    karray.push_back(std::make_pair("", key));
                    m_keyboard.push_back(std::make_pair("", karray));
                    karray.clear();
                }
                break;
        }
    }

    inline void clear(){m_keyboard.clear();}
    inline const pt::ptree& keyboard(){return m_keyboard;}

private:
    pt::ptree m_keyboard;
};

#endif // TKEYBOARD_H
