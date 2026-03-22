#pragma once

#include <windows.h>

class VTableHookManager {
public:
    VTableHookManager(void** vTable, unsigned short numFuncs) : m_vTable(vTable), m_numberFuncs(numFuncs) {
        m_originalFuncs = new void*[m_numberFuncs];
        for (unsigned short i = 0; i < m_numberFuncs; ++i) {
            m_originalFuncs[i] = GetFunctionAddyByIndex(i);
        }
    }

    ~VTableHookManager() {
        delete[] m_originalFuncs;
    }

    void* GetFunctionAddyByIndex(unsigned short index) const {
        if (index < m_numberFuncs) {
            return m_vTable[index];
        }
        return nullptr;
    }

    void* Hook(unsigned short index, void* ourFunction) {
        uintptr_t original = 0;
        if (!DoHook(index, true, ourFunction, &original)) {
            return nullptr;
        }
        return reinterpret_cast<void*>(original);
    }

    bool Unhook(unsigned short index) {
        return DoHook(index, false, nullptr, nullptr);
    }

private:
    bool DoHook(unsigned short index, bool hook, void* ourFunction, uintptr_t* original) {
        if (index >= m_numberFuncs) {
            return false;
        }

        DWORD oldProtect = 0;
        if (!VirtualProtect(m_vTable + index, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect)) {
            return false;
        }

        if (hook) {
            if (ourFunction == nullptr || original == nullptr) {
                VirtualProtect(m_vTable + index, sizeof(void*), oldProtect, &oldProtect);
                return false;
            }
            *original = reinterpret_cast<uintptr_t>(m_vTable[index]);
            m_vTable[index] = ourFunction;
        } else {
            m_vTable[index] = m_originalFuncs[index];
        }

        VirtualProtect(m_vTable + index, sizeof(void*), oldProtect, &oldProtect);
        return true;
    }

    void** m_vTable = nullptr;
    unsigned short m_numberFuncs = 0;
    void** m_originalFuncs = nullptr;
};
