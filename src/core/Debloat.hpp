#pragma once
#include <string>
#include <vector>

namespace core {
struct OptionalApp {
    std::wstring id;
    std::wstring name;
    std::wstring description;
};

const std::vector<OptionalApp>& GetOptionalApps();
bool RemoveCurrentUserApp(const std::wstring& packageId, std::wstring& message);
}
