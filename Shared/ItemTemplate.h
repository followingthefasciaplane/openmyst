#pragma once

#include "ItemDefiner.h"
#include <vector>
#include <map>

// Item template database - holds all item definitions loaded from DB
class ItemTemplate
{
public:
    static ItemTemplate& instance();

    void addItem(const ItemDefiner& item);
    const ItemDefiner* getItem(int entry) const;
    bool hasItem(int entry) const;

    const std::map<int, ItemDefiner>& getItems() const { return m_items; }

private:
    ItemTemplate() = default;
    std::map<int, ItemDefiner> m_items;
};

#define sItemTemplate ItemTemplate::instance()
