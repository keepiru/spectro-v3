# Coding Standards

This document describes the coding standards and conventions used in the Spectro-v3 project.

## General Principles
- C++23
- Follow the Mozilla style guide
- Modern C++ idioms (STL containers, smart pointers, RAII)

## File Organization

### Source files
- One concrete class per file
- Pure virtual interfaces defined in the same header, above implementation

### Classes
- Order class members: public, protected, private
- Within each section, order: types, constants, constructors, methods, data members
- Prefer composition over inheritance

### Types and Type Safety
- See `audio_types.h` for semantic strong types used throughout

### Error Handling
- Use static safety to prevent errors before they happen
- Prefer to propagate errors in return values
- Do not use exceptions for flow control.
- Use exceptions for fatal errors.
    - Only catch exceptions in tests.
    - Let exceptions propagate unless you can meaningfully handle them.

