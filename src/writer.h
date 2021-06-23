#pragma once

#include <boost/sml.hpp>
#include <uvw.hpp>


template <typename File>
class Writer
{
public:
    Writer(std::shared_ptr<File>);

    void push(const std::string&);

private:
    std::shared_ptr<File> file;
};


template <typename File>
Writer<File>::Writer(std::shared_ptr<File> f)
    :
      file{std::move(f)}
{}

template <typename File>
void Writer<File>::push(const std::string&) {

}
