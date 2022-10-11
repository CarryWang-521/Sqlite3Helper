#ifndef BINDER_PARSER_H
#define BINDER_PARSER_H
#include <string>
#include "RawBinderBase.h"

namespace CommonSqlite {
    class Binder : public RawBinderBase<std::pair<std::string, int64_t>, std::pair<std::string, std::string>> {

    };
    class Parser : public RawBinderBase<std::pair<std::string, int64_t>, std::pair<std::string, std::string>> {

    };
}
#endif
