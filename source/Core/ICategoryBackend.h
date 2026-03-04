#pragma once

#include <cstdint>

class ICategoryBackend
{
public:
    virtual ~ICategoryBackend() = default;

    // Wywoływane przy pierwszym użyciu backendu
    virtual bool Initialize() = 0;

    // Aktualizacja danych (wywoływane przez Core)
    virtual void Update() = 0;

    // Pobranie wartości dla konkretnego urządzenia i metryki
    virtual double GetValue(
        uint32_t deviceIndex,
        uint32_t metricId
    ) = 0;

    // Opcjonalnie: ile urządzeń w danej kategorii
    virtual uint32_t GetDeviceCount() const = 0;
};