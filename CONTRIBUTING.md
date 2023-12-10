# Contributing to CleverCoffee

Thank you for considering contributing to the project. To ensure consistency and maintainability, please follow these style guidelines when submitting code changes.

## Code Style Guidelines

### Coding Standards

- **Indentation**: Use 4 spaces for indentation.
- **Curly Braces**: Place opening curly braces on the same line as the statement.
  ```cpp
  // Example
  if (condition) {
      // code
  }
  else {
      // code
  }
  ```
- **Capitalization**: Follow C++ style capitalization.
  ```cpp
  // Example
  int myVariable;
  
  void myFunction() {
      // code
  }
  ```
- **Operators**: Use spaces around operators.
  ```cpp
  // Example
  int sum = a + b;
  
  if (x == y) {
      // code
  }
  ```

### Code Organization

**Blank Lines**: Include a blank line before and after a code block.
  ```cpp
  // Example
  void functionA() {
      // code
  }

  void functionB() {
      // code
  }
  ```

### Encapsulation

- **Separate Files or Classes**: Encapsulate significant code changes in separate header files or classes if applicable. This helps maintain a modular and organized codebase, improving readability and reusability.
- **Include guards**: Use the #pragma directive instead of include guards:
  ```cpp
  #pragma once
  // code

  // instead of
  #ifndef MENU_H
  #define MENU_H
    // code
  #endif
  ```

### Comments

**Meaningful Comments**: Write clear and descriptive comments. Comments should explain the 'why' behind the code, not just reiterate the code itself. For instance, the following example doesn't provide any additional information:
  ```cpp
  // increment some values
  i++;
  j++;
  ```

## Submitting Changes

1. Fork the repository and create a new branch for your changes.
2. Ensure your code follows the outlined style guidelines.
3. Make sure to include clear commit messages explaining the purpose of the changes.
4. Open a pull request with a descriptive title and detailed information about the changes made.

## Code Review Process

All contributions will be reviewed to ensure compliance with the project's guidelines. Be prepared to address any feedback or suggestions for improvement during the review process.

Thank you for your contributions to CleverCoffee!
