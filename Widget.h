#ifndef _WIDGET_H_
#define _WIDGET_H_

#include <vector>
#include <string>

#include "memory.h"

class Widget {
protected:
    MyWeakPtr<Widget> parent;
    std::vector<MyUniquePtr<Widget>> children;

public:
    Widget() noexcept = default;
    Widget(const MySharedPtr<Widget>& parent) noexcept : parent(parent) {
        parent->addChild(this);
    };
    virtual ~Widget() = default;

    virtual std::string getType() const = 0;

    const MyWeakPtr<Widget>& getParent() const {
        return parent;
    }

    void addChild(Widget* child) {
        children.emplace_back(std::move(child));
    }

    const std::vector<MyUniquePtr<Widget>>& getChildren() const {
        return children;
    }
};

class TabWidget : public Widget {
public:
    TabWidget(const MySharedPtr<Widget>& parent) : Widget(parent) {};
    std::string getType() const override {
        return "TabWidget";
    }
};

class CalendarWidget : public Widget {
public:
    CalendarWidget(const MySharedPtr<Widget>& parent) : Widget(parent) {};
    std::string getType() const override {
        return "CalendarWidget";
    }
};

#endif 