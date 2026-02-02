// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once

#include <QObject>

// Forward declarations
class Settings;

/// @brief Controller for application settings
///
/// Coordinates the interaction between SettingsPanel (view) and Settings (model).
class SettingsController : public QObject
{
    Q_OBJECT

  public:
    /// @brief Constructor
    /// @param aSettings Reference to the settings model
    /// @param aParent Qt parent object (optional)
    explicit SettingsController(Settings& aSettings, QObject* aParent = nullptr);

  private:
    Settings& mSettings;
};
