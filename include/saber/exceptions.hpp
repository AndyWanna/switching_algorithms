// exceptions.hpp header file
// This file contains all customized exceptions defined for this project.

#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP

#include <exception>
#include <string>

namespace saber {
// DisabledMemberException exception
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct DisabledMemberException : public std::exception {
 private:
  std::string _msg;
 public:
  explicit DisabledMemberException(std::string msg) : _msg(std::string("DisabledMemberError: ") + std::move(msg)) {}
  const char *what() const noexcept override {
    return _msg.c_str();
  }
};// class DisabledMemberException
// OutOfBoundary exception
// ////////////////////////////////////////////////////////////////////////////////////////////////////////
struct OutOfBoundaryException : public std::exception {
 private:
  std::string _msg;
 public:
  explicit OutOfBoundaryException(std::string msg) : _msg(std::string("OutOfBoundaryError: ") + std::move(msg)) {}
  const char *what() const noexcept override {
    return _msg.c_str();
  }
};// struct OutOfBoundary
// MissingComponent exception
// ////////////////////////////////////////////////////////////////////////////////////////////////////////
struct MissingComponentException : public std::exception {
 private:
  std::string _msg;
 public:
  explicit MissingComponentException(std::string msg) : _msg(std::string("MissingComponentError: ") + std::move(msg)) {}
  const char *what() const noexcept override {
    return _msg.c_str();
  }
};// struct MissingComponentException
// ReInitialization exception
// ////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ReInitializationException : public std::exception {
 private:
  std::string _msg;
 public:
  explicit ReInitializationException(std::string msg) : _msg(std::string("ReInitializationError: ") + std::move(msg)) {}
  const char *what() const noexcept override {
    return _msg.c_str();
  }
};// struct ReInitializationException
// EmptyName exception
// ////////////////////////////////////////////////////////////////////////////////////////////////////////
struct EmptyNameException : public std::exception {
 private:
  std::string _msg;
 public:
  explicit EmptyNameException(std::string msg) : _msg(std::string("EmptyNameError: ") + std::move(msg)) {}
  const char *what() const noexcept override {
    return _msg.c_str();
  }
};// struct EmptyNameException
// MissingArgument exception
// ////////////////////////////////////////////////////////////////////////////////////////////////////////
struct MissingArgumentException : public std::exception {
 private:
  std::string _msg;
 public:
  explicit MissingArgumentException(std::string msg) : _msg(std::string("MissingArgumentError: ") + std::move(msg)) {}
  const char *what() const noexcept override {
    return _msg.c_str();
  }
};// struct MissingArgumentException
// UnknownParameter exception
// ////////////////////////////////////////////////////////////////////////////////////////////////////////
struct UnknownParameterException : public std::exception {
 private:
  std::string _msg;
 public:
  explicit UnknownParameterException(std::string msg) : _msg(std::string("UnknownParameterError: ") + std::move(msg)) {}
  const char *what() const noexcept override {
    return _msg.c_str();
  }
};// struct UnknownParameter
// EmptyArgument exception
// ////////////////////////////////////////////////////////////////////////////////////////////////////////
struct EmptyArgumentException : public std::exception {
 private:
  std::string _msg;
 public:
  explicit EmptyArgumentException(std::string msg) : _msg(std::string("EmptyArgumentError: ") + std::move(msg)) {}
  const char *what() const noexcept override {
    return _msg.c_str();
  }
};// struct EmptyArgumentException
// NonEmptyInitialization exception
// ////////////////////////////////////////////////////////////////////////////////////////////////////////
struct NonEmptyInitializationException : public std::exception {
 private:
  std::string _msg;
 public:
  explicit NonEmptyInitializationException(std::string msg) : _msg(std::string("NonEmptyInitializationError: ") + std::move(msg)) {}
  const char *what() const noexcept override {
    return _msg.c_str();
  }
};// struct NonEmptyInitializationException
// DeleteNonInitialized exception
// ////////////////////////////////////////////////////////////////////////////////////////////////////////
struct DeleteNonInitializedException : public std::exception {
 private:
  std::string _msg;
 public:
  explicit DeleteNonInitializedException(std::string msg) : _msg(std::string("DeleteNonInitializedError: ") + std::move(msg)) {}
  const char *what() const noexcept override {
    return _msg.c_str();
  }
};// struct DeleteNonInitializedException
} // namespace saber
#endif // EXCEPTIONS_HPP
