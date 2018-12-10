#include "json.h"
#include <algorithm>

namespace json {

template<class T>
struct Defaults {
  static T value_;
};
template<class T>
T Defaults<T>::value_;

Value::Value(Type type)
  : type_(tUndefined)
{
  setType(type);
}
Value::Value(bool val)
  : type_(tUndefined)
{
  setType(tBoolean);
  bool_ = val;
}
//Value::Value(int val)
//  : type_(tUndefined)
//{
//  setType(tInteger);
//  int_ = val;
//}
//Value::Value(unsigned int val)
//  : type_(tUndefined)
//{
//  if (val > 0x7FFFFFFF) {
//    setType(tNumber);
//    number_ = static_cast<double>(val);
//  } else {
//    setType(tInteger);
//    int_ = static_cast<int>(val);
//  }
//}
Value::Value(sint32 val)
  : type_(tUndefined)
{
  setType(tInteger);
  int_ = static_cast<int>(val);
}
Value::Value(uint32 val)
  : type_(tUndefined)
{
  if (val > 0x7FFFFFFF) {
    setType(tNumber);
    number_ = static_cast<double>(val);
  } else {
    setType(tInteger);
    int_ = static_cast<int>(val);
  }
}
Value::Value(sint64 val)
  : type_(tUndefined)
{
  if (val > 0x7FFFFFFFLL || val < -0x80000000LL) {
    setType(tNumber);
    number_ = static_cast<double>(val);
  } else {
    setType(tInteger);
    int_ = static_cast<int>(val);
  }
}
Value::Value(uint64 val)
  : type_(tUndefined)
{
  if (val > 0x7FFFFFFFULL) {
    setType(tNumber);
    number_ = static_cast<double>(val);
  } else {
    setType(tInteger);
    int_ = static_cast<int>(val);
  }
}
Value::Value(double val)
  : type_(tUndefined)
{
  setType(tNumber);
  number_ = val;
}
Value::Value(std::string const& val)
  : type_(tUndefined)
{
  setType(tString);
  *string_ = val;
}
Value::Value(char const* val)
  : type_(tUndefined)
{
  if (val) {
    setType(tString);
    *string_ = val;
  } else {
    setType(tNull);
  }
}
Value::Value(Value const& rhs)
: type_(rhs.type_)
{
  switch (type_) {
  case tString:
    string_ = new std::string(*rhs.string_);
    break;
  case tObject:
    map_ = new Map(*rhs.map_);
    break;
  case tArray:
    array_ = new Array(*rhs.array_);
    break;
  case tInteger:
    int_ = rhs.int_;
    break;
  case tNumber:
    number_ = rhs.number_;
    break;
  case tBoolean:
    bool_ = rhs.bool_;
    break;
  default:
    break;
  }
}

Value& Value::operator=(Value const& rhs) {
  if (&rhs == this) return *this;
  switch (rhs.type_) {
  case tString:
  {
    std::string* tmp = new std::string(*rhs.string_);
    clear();
    type_ = tString;
    string_ = tmp;
    break;
  }
  case tObject:
  {
    Map* tmp = new Map(*rhs.map_);
    clear();
    type_ = tObject;
    map_ = tmp;
    break;
  }
  case tArray:
  {
    Array* tmp = new Array(*rhs.array_);
    clear();
    type_ = tArray;
    array_ = tmp;
    break;
  }
  case tInteger:
  {
    int tmp = rhs.int_;
    clear();
    type_ = tInteger;
    int_ = tmp;
    break;
  }
  case tNumber:
  {
    double tmp = rhs.number_;
    clear();
    type_ = tNumber;
    number_ = tmp;
    break;
  }
  case tBoolean:
  {
    bool tmp = rhs.bool_;
    clear();
    type_ = tBoolean;
    bool_ = tmp;
    break;
  }
  default:
    break;
  }
  return *this;
}

void Value::clear() {
  switch (type_) {
  case tString:
    delete string_;
    break;
  case tObject:
    delete map_;
    break;
  case tArray:
    delete array_;
    break;
  default:
    break;
  }
  type_ = tUndefined;
}
Value& Value::setType(Type type) {
  if (type == type_) return *this;
  clear();
  type_ = type;
  switch (type_) {
  case tString:
    string_ = new std::string();
    break;
  case tObject:
    map_ = new Map();
    break;
  case tArray:
    array_ = new Array();
    break;
  default:
    break;
  }
  return *this;
}

// tBoolean
bool Value::getBoolean() const {
  return (type_ == tBoolean ? bool_ : false);
}
Value& Value::setBoolean(bool data) {
  setType(tBoolean);
  bool_ = data;
  return *this;
}

std::string const& Value::getString() const {
  return (type_ == tString ? *string_ : Defaults<std::string>::value_);
}
Value& Value::setString(char const* data) {
  setType(tString);
  *string_ = data;
  return *this;
}
Value& Value::setString(std::string const& data) {
  setType(tString);
  *string_ = data;
  return *this;
}

// tNumber
bool Value::isInteger() const {
  switch (type_) {
  case tInteger: return true;
  case tNumber: return (int)number_ == number_;
  default: return false;
  }
}
int Value::getInteger() const {
  if (!isInteger()) return 0;
  switch (type_) {
  case tInteger: return int_;
  case tNumber: return static_cast<int>(number_);
  default: return 0;
  }
}
double Value::getNumber() const {
  switch (type_) {
  case tInteger: return static_cast<double>(int_);
  case tNumber: return number_;
  default: return 0;
  }
}
Value& Value::setInteger(int data) {
  setType(tInteger);
  int_ = data;
  return *this;
}
Value& Value::setNumber(double data) {
  setType(tNumber);
  number_ = data;
  return *this;
}

// tObject
Value::Map const& Value::getMap() const {
  return (type_ == tObject ? *map_ : Defaults<Map>::value_);
}
bool Value::has(std::string const& name) const {
  if (type_ != tObject) return false;
  return map_->find(name) != map_->end();
}
bool Value::has(char const* name) const {
  if (type_ != tObject) return false;
  return map_->find(name) != map_->end();
}
Value const* Value::get(std::string const& name) const {
  if (type_ != tObject) return nullptr;
  auto it = map_->find(name);
  return (it != map_->end() ? &it->second : nullptr);
}
Value* Value::get(std::string const& name) {
  if (type_ != tObject) return nullptr;
  auto it = map_->find(name);
  return (it != map_->end() ? &it->second : nullptr);
}
Value const* Value::get(char const* name) const {
  if (type_ != tObject) return nullptr;
  auto it = map_->find(name);
  return (it != map_->end() ? &it->second : nullptr);
}
Value* Value::get(char const* name) {
  if (type_ != tObject) return nullptr;
  auto it = map_->find(name);
  return (it != map_->end() ? &it->second : nullptr);
}
Value& Value::insert(std::string const& name, Value const& data) {
  setType(tObject);
  return (*map_)[name] = data;
}
Value& Value::insert(char const* name, Value const& data) {
  setType(tObject);
  return (*map_)[name] = data;
}
void Value::remove(std::string const& name) {
  if (type_ == tObject) map_->erase(name);
}
void Value::remove(char const* name) {
  if (type_ == tObject) map_->erase(name);
}
Value const& Value::operator[](std::string const& name) const {
  Value const* ptr = get(name);
  return (ptr ? *ptr : Defaults<Value>::value_);
}
Value const& Value::operator[](char const* name) const {
  Value const* ptr = get(name);
  return (ptr ? *ptr : Defaults<Value>::value_);
}
Value& Value::operator[](std::string const& name) {
  setType(tObject);
  return (*map_)[name];
}
Value& Value::operator[](char const* name) {
  setType(tObject);
  return (*map_)[name];
}

// tArray
Value::Array const& Value::getArray() const {
  return (type_ == tArray ? *array_ : Defaults<Array>::value_);
}
uint32 Value::length() const {
  return (type_ == tArray ? array_->size() : 0);
}
Value const* Value::at(uint32 i) const {
  if (type_ != tArray || i >= array_->size()) return nullptr;
  return &(*array_)[i];
}
Value* Value::at(uint32 i) {
  if (type_ != tArray || i >= array_->size()) return nullptr;
  return &(*array_)[i];
}
Value& Value::insert(uint32 i, Value const& data) {
  setType(tArray);
  return *array_->insert(array_->begin() + std::min<uint32>(i, array_->size()), data);
}
Value& Value::append(Value const& data) {
  setType(tArray);
  array_->push_back(data);
  return array_->back();
}
void Value::remove(uint32 i) {
  if (type_ != tArray || i >= array_->size()) return;
  array_->erase(array_->begin() + i);
}
void Value::resize(size_t size) {
  setType(tArray);
  array_->resize(size);
}
Value const& Value::operator[](int i) const {
  if (type_ != tArray || i < 0 || static_cast<size_t>(i) >= array_->size()) return Defaults<Value>::value_;
  return (*array_)[i];
}
Value& Value::operator[](int i) {
  setType(tArray);
  if (i < 0) i = 0;
  if (array_->size() <= static_cast<size_t>(i)) {
    array_->resize(i + 1);
  }
  return (*array_)[i];
}

Value::Iterator Value::begin() {
  switch (type_) {
  case tObject:
    return Iterator(map_->begin());
  case tArray:
    return Iterator(array_->begin());
  default:
    return Iterator();
  }
}
Value::Iterator Value::end() {
  switch (type_) {
  case tObject:
    return Iterator(map_->end());
  case tArray:
    return Iterator(array_->end());
  default:
    return Iterator();
  }
}
Value::ConstIterator Value::begin() const {
  switch (type_) {
  case tObject:
    return ConstIterator(map_->begin());
  case tArray:
    return ConstIterator(array_->begin());
  default:
    return ConstIterator();
  }
}
Value::ConstIterator Value::end() const {
  switch (type_) {
  case tObject:
    return ConstIterator(map_->end());
  case tArray:
    return ConstIterator(array_->end());
  default:
    return ConstIterator();
  }
}

class Tokenizer {
  File* file;
  bool strict;
public:
  uint32 line;
  uint32 col;
  int chr;
  int move() {
    int old = chr;
    chr = file->getc();
    if (chr == '\n') ++line;
    if (chr == '\r' || chr == '\n') col = 0;
    else ++col;
    return old;
  }

  enum State {tEnd, tSymbol, tInteger, tNumber, tString, tIdentifier, tError = -1};
  State state;

  int valInteger;
  double valNumber;
  std::string value;

  Tokenizer(File* file, bool strict)
    : file(file)
    , strict(strict)
    , line(0)
    , col(0)
    , chr(file->getc())
  {}

  State next();
};

Tokenizer::State Tokenizer::next() {
  while (chr != EOF && isspace(chr)) {
    move();
  }
  if (chr == EOF) {
    return state = tEnd;
  }
  value.clear();
  if (chr == '"' || (!strict && chr == '\'')) {
    char init = chr;
    state = tString;
    move();
    while (chr != init && chr != EOF) {
      if (chr == '\\') {
        move();
        switch (chr) {
        case '\'':
        case '"':
        case '\\':
        case '/':
          value.push_back(move());
          break;
        case 'b':
          value.push_back('\b');
          move();
          break;
        case 'f':
          value.push_back('\f');
          move();
          break;
        case 'n':
          value.push_back('\n');
          move();
          break;
        case 'r':
          value.push_back('\r');
          move();
          break;
        case 't':
          value.push_back('\t');
          move();
          break;
        case 'u': {
          move();
          uint32 cp = 0;
          for (int i = 0; i < 4; ++i) {
            if (chr >= '0' && chr <= '9') {
              cp = cp * 16 + (move() - '0');
            } else if (chr >= 'a' && chr <= 'f') {
              cp = cp * 16 + (move() - 'a') + 10;
            } else if (chr >= 'A' && chr <= 'F') {
              cp = cp * 16 + (move() - 'A') + 10;
            } else {
              value = "invalid hex digit";
              return state = tError;
            }
          }
          if (cp <= 0x7F) {
            value.push_back(cp);
          } else if (cp <= 0x7FF) {
            value.push_back(0xC0 | ((cp >> 6) & 0x1F));
            value.push_back(0x80 | (cp & 0x3F));
          } else {
            value.push_back(0xE0 | ((cp >> 12) & 0x0F));
            value.push_back(0x80 | ((cp >> 6) & 0x3F));
            value.push_back(0x80 | (cp & 0x3F));
          }
          break;
        }
        default:
          value = "invalid escape sequence";
          return state = tError;
        }
      } else {
        value.push_back(move());
      }
    }
    move();
  } else if (chr == '-' || (chr >= '0' && chr <= '9') || (!strict && (chr == '.' || chr == '+'))) {
    state = tInteger;
    if (chr == '-' || (!strict && chr == '+')) {
      value.push_back(move());
    }
    if (chr == '0') {
      value.push_back(move());
    } else if (chr >= '1' && chr <= '9') {
      while (chr >= '0' && chr <= '9') {
        value.push_back(move());
      }
    } else if (strict || chr != '.') {
      value = "invalid number";
      return state = tError;
    }
    if (chr == '.') {
      state = tNumber;
      value.push_back(move());
      if ((chr < '0' || chr > '9') && strict) {
        value = "invalid number";
        return state = tError;
      }
      while (chr >= '0' && chr <= '9') {
        value.push_back(move());
      }
    }
    if (chr == 'e' || chr == 'E') {
      state = tNumber;
      value.push_back(move());
      if (chr == '-' || chr == '+') {
        value.push_back(move());
      }
      if (chr < '0' || chr > '9') {
        value = "invalid number";
        return state = tError;
      }
      while (chr >= '0' && chr <= '9') {
        value.push_back(move());
      }
    }
    valNumber = atof(value.c_str());
    if (state == tInteger) {
      valInteger = int(valNumber);
      if (double(valInteger) != valNumber) {
        state = tNumber;
      }
    }
  } else if (chr == '{' || chr == '}' || chr == '[' || chr == ']' || chr == ':' || chr == ',' || chr == '(' || chr == ')') {
    state = tSymbol;
    value.push_back(move());
  } else if ((chr >= 'a' && chr <= 'z') || (chr >= 'A' && chr <= 'Z') || chr == '_') {
    state = tIdentifier;
    while ((chr >= 'a' && chr <= 'z') || (chr >= 'A' && chr <= 'Z') || (chr >= '0' && chr <= '9') || chr == '_') {
      value.push_back(move());
    }
  } else if (chr == '/') {
    move();
    if (chr == '/') {
      while (chr != '\n' && chr != EOF) {
        move();
      }
      return next();
    } else if (chr == '*') {
      move();
      bool star = false;
      while (chr != EOF && (!star || chr != '/')) {
        star = (chr == '*');
        move();
      }
      if (chr == EOF) {
        value = "unterminated comment";
        return state = tError;
      }
      move();
      return next();
    } else {
      value = "unexpected symbol '/'";
      return state = tError;
    }
  } else {
    value = "unexpected symbol '";
    value.push_back(chr);
    value.push_back('\'');
    return state = tError;
  }
  return state;
}

bool parse(File file, Visitor* visitor, int mode, std::string* func) {
  enum State{sValue, sKey, sColon, sNext, sEnd} state = sValue;
  std::vector<Value::Type> objStack;
  bool topEmpty = true;
  Tokenizer tok(&file, mode == mJSON);
  if (mode == mJSCall) {
    if (func) func->clear();
    while (tok.chr != EOF && tok.chr != '(') {
      if (func) func->push_back(tok.chr);
      tok.move();
    }
    if (tok.chr != '(') {
      visitor->onError(tok.line, tok.col, "expected '('");
      return false;
    }
    tok.move();
  }
  tok.next();
  while (state != sEnd) {
    if (tok.state == Tokenizer::tError) {
      visitor->onError(tok.line, tok.col, tok.value);
      return false;
    }
    bool advance = true;
    switch (state) {
    case sValue:
      if (tok.state == Tokenizer::tInteger) {
        if (!visitor->onInteger(tok.valInteger)) return false;
      } else if (tok.state == Tokenizer::tNumber) {
        if (!visitor->onNumber(tok.valNumber)) return false;
      } else if (tok.state == Tokenizer::tString) {
        if (!visitor->onString(tok.value)) return false;
      } else if (tok.state == Tokenizer::tIdentifier) {
        if (tok.value == "null") {
          if (!visitor->onNull()) return false;
        } else if (tok.value == "true") {
          if (!visitor->onBoolean(true)) return false;
        } else if (tok.value == "false") {
          if (!visitor->onBoolean(false)) return false;
        } else {
          visitor->onError(tok.line, tok.col, "unexpected identifier " + tok.value);
          return false;
        }
      } else if (tok.state == Tokenizer::tSymbol) {
        if (tok.value == "{") {
          if (!visitor->onOpenMap()) return false;
          objStack.push_back(Value::tObject);
          topEmpty = true;
          state = sKey;
          break;
        } else if (tok.value == "[") {
          if (!visitor->onOpenArray()) return false;
          objStack.push_back(Value::tArray);
          topEmpty = true;
          state = sValue;
          break;
        } else if ((mode != mJSON || topEmpty) && tok.value == "]" && !objStack.empty() && objStack.back() == Value::tArray) {
          state = sNext;
          advance = false;
          break;
        } else {
          visitor->onError(tok.line, tok.col, "unexpected symbol '" + tok.value + "'");
          return false;
        }
      } else {
        visitor->onError(tok.line, tok.col, "value expected");
        return false;
      }
      topEmpty = false;
      if (objStack.empty()) {
        state = sEnd;
      } else {
        state = sNext;
      }
      break;
    case sKey:
      if (tok.state == Tokenizer::tString) {
        if (!visitor->onMapKey(tok.value)) return false;
        state = sColon;
      } else if (mode != mJSON && tok.state == Tokenizer::tIdentifier) {
        if (!visitor->onMapKey(tok.value)) return false;
        state = sColon;
      } else if (mode != mJSON && (tok.state == Tokenizer::tNumber || tok.state == Tokenizer::tInteger)) {
        if (!visitor->onMapKey(tok.value)) return false;
        state = sColon;
      } else if ((mode != mJSON || topEmpty) && tok.state == Tokenizer::tSymbol && tok.value == "}") {
        state = sNext;
        advance = false;
      } else {
        visitor->onError(tok.line, tok.col, "object key expected");
        return false;
      }
      break;
    case sColon:
      if (tok.state == Tokenizer::tSymbol && tok.value == ":") {
        state = sValue;
      } else {
        visitor->onError(tok.line, tok.col, "':' expected");
        return false;
      }
      break;
    case sNext:
      if (tok.state == Tokenizer::tSymbol) {
        if (tok.value == ",") {
          if (objStack.back() == Value::tObject) {
            state = sKey;
          } else {
            state = sValue;
          }
        } else if (tok.value == "}") {
          if (objStack.back() != Value::tObject) {
            visitor->onError(tok.line, tok.col, "mismatched '}'");
            return false;
          }
          if (!visitor->onCloseMap()) return false;
        } else if (tok.value == "]") {
          if (objStack.back() != Value::tArray) {
            visitor->onError(tok.line, tok.col, "mismatched ']'");
            return false;
          }
          if (!visitor->onCloseArray()) return false;
        } else {
          visitor->onError(tok.line, tok.col, "unexpected symbol '" + tok.value + "'");
          return false;
        }
        if (state == sNext) {
          objStack.pop_back();
          topEmpty = false;
          if (objStack.empty()) {
            advance = false;
            state = sEnd;
          } else {
            state = sNext;
          }
        }
      } else {
        if (objStack.back() == Value::tObject) {
          visitor->onError(tok.line, tok.col, "'}' or ',' expected");
        } else {
          visitor->onError(tok.line, tok.col, "']' or ',' expected");
        }
        return false;
      }
      break;
    default:
      visitor->onError(tok.line, tok.col, "internal error");
      return false;
    }
    if (advance) {
      tok.next();
    }
  }
  if (mode == mJSCall) {
    if (tok.next() != Tokenizer::tSymbol || tok.value != ")") {
      visitor->onError(tok.line, tok.col, "expected ')'");
      return false;
    }
    tok.move();
    if (tok.chr == ';') tok.move();
    //while (tok.chr != EOF && isspace(tok.chr)) tok.move();
    //if (tok.chr != EOF) {
    //  visitor->onError(tok.line, tok.col, fmtstring("unexpected symbol '%c'", (char)tok.chr));
    //  return false;
    //}
  } else {
    //if (tok.next() != Tokenizer::tEnd) {
    //  visitor->onError(tok.line, tok.col, fmtstring("unexpected symbol '%c'", (char)tok.chr));
    //  return false;
    //}
  }
  file.seek(-1, SEEK_CUR);
  return visitor->onEnd();
}

BuilderVisitor::BuilderVisitor(Value& value, bool throwExceptions)
  : Visitor(throwExceptions)
  , value_(value)
  , state_(sStart)
{}

bool BuilderVisitor::openComplexValue(Value::Type type) {
  switch (state_) {
  case sStart:
    stack_.push_back(&value_.setType(type));
    break;
  case sArrayValue:
    stack_.push_back(&stack_.back()->append(type));
    break;
  case sMapValue:
    stack_.push_back(&stack_.back()->insert(key_, type));
    state_ = sMapKey;
    break;
  default:
    return false;
  }
  return true;
}

bool BuilderVisitor::closeComplexValue() {
  stack_.pop_back();
  if (!stack_.empty()) {
    switch (stack_.back()->type()) {
    case Value::tObject:
      state_ = sMapKey;
      break;
    case Value::tArray:
      state_ = sArrayValue;
      break;
    default:
      return false;
    }
  } else {
    state_ = sFinish;
  }
  return true;
}

bool parse(File file, Value& value, int mode, std::string* func, bool throwExceptions) {
  if (!file) return false;
  BuilderVisitor builder(value, throwExceptions);
  value.clear();
  return parse(file, &builder, mode, func);
}

bool Value::walk(Visitor* visitor) const {
  switch (type_) {
  case tUndefined:
  case tNull:
    return visitor->onNull();
  case tBoolean:
    return visitor->onBoolean(bool_);
  case tString:
    return visitor->onString(*string_);
  case tInteger:
    return visitor->onInteger(int_);
  case tNumber:
    return visitor->onNumber(number_);
  case tObject:
    if (!visitor->onOpenMap()) return false;
    for (auto it = map_->begin(); it != map_->end(); ++it) {
      if (!visitor->onMapKey(it->first)) return false;
      if (!it->second.walk(visitor)) return false;
    }
    return visitor->onCloseMap();
  case tArray:
    if (!visitor->onOpenArray()) return false;
    for (auto it = array_->begin(); it != array_->end(); ++it) {
      if (!it->walk(visitor)) return false;
    }
    return visitor->onCloseArray();
  default:
    return false;
  }
}

WriterVisitor::WriterVisitor(File const& file, int mode, char const* func)
  : file_(file)
  , mode_(mode)
  , escape_(false)
  , empty_(true)
  , object_(false)
{
  if (mode == mJSCall) {
    file_.printf("%s(", func ? func : "");
  }
}

void WriterVisitor::onValue() {
  if (!object_ && !empty_) {
    file_.putc(',');
  }
  if (!object_ && !curIndent_.empty()) {
    file_.printf("\n%s", curIndent_.c_str());
  }
  empty_ = false;
  object_ = false;
}

void WriterVisitor::openValue(char chr) {
  onValue();
  empty_ = true;
  file_.putc(chr);
  curIndent_.append(indent_);
}
void WriterVisitor::closeValue(char chr) {
  curIndent_.resize(curIndent_.size() - indent_.size());
  if (!empty_ && !indent_.empty()) {
    if (mode_ != mJSON) file_.putc(',');
    file_.printf("\n%s", curIndent_.c_str());
  }
  file_.putc(chr);
  empty_ = false;
}

void WriterVisitor::writeString(std::string const& str) {
  file_.putc('"');
  for (uint32 pos = 0; pos < str.length(); ++pos) {
    uint8 chr = static_cast<uint8>(str[pos]);
    switch (chr) {
    case '"':
      file_.printf("\\\"");
      break;
    case '\\':
      file_.printf("\\\\");
      break;
    case '\b':
      file_.printf("\\b");
      break;
    case '\f':
      file_.printf("\\f");
      break;
    case '\n':
      file_.printf("\\n");
      break;
    case '\r':
      file_.printf("\\r");
      break;
    case '\t':
      file_.printf("\\t");
      break;
    default:
      if (chr < 32) {
        file_.printf("\\u%04X", (int)chr);
      } else if (chr <= 0x7F || !escape_) {
        file_.putc(chr);
      } else {
        uint8 hdr = chr;
        uint32 mask = 0x3F;
        uint32 cp = chr;
        while ((hdr & 0xC0) == 0xC0 && pos < str.length() - 1) {
          chr = str[++pos];
          cp = (cp << 6) | (chr & 0x3F);
          mask = (mask << 5) | 0x1F;
          hdr <<= 1;
        }
        cp &= mask;
        file_.printf("\\u%04X", cp & 0xFFFF);
      }
    }
  }
  file_.putc('"');
}

bool WriterVisitor::onNull() {
  onValue();
  file_.printf("null");
  return true;
}
bool WriterVisitor::onBoolean(bool val) {
  onValue();
  file_.printf(val ? "true" : "false");
  return true;
}
bool WriterVisitor::onInteger(int val) {
  onValue();
  file_.printf("%d", val);
  return true;
}
bool WriterVisitor::onNumber(double val) {
  onValue();
  file_.printf("%.14g", val);
  return true;
}
bool WriterVisitor::onString(std::string const& val) {
  onValue();
  writeString(val);
  return true;
}
bool WriterVisitor::onMapKey(std::string const& key) {
  onValue();
  object_ = true;
  bool safe = (mode_ != mJSON) &&
              (!key.empty() && ((key[0] >= 'a' && key[0] <= 'z')
                             || (key[0] >= 'A' && key[0] <= 'Z')
                             || key[0] == '_'));
  for (uint32 pos = 1; pos < key.length() && safe; ++pos) {
    if ((key[pos] < 'a' || key[pos] > 'z') &&
        (key[pos] < 'A' || key[pos] > 'Z') &&
        (key[pos] < '0' || key[pos] > '9') && key[pos] != '_') {
      safe = false;
    }
  }
  if (safe) {
    file_.printf("%s", key.c_str());
  } else {
    writeString(key);
  }
  file_.putc(':');
  if (!indent_.empty()) file_.putc(' ');
  return true;
}
bool WriterVisitor::onEnd() {
  if (mode_ == mJSCall) file_.printf(");");
  if (!indent_.empty()) file_.putc('\n');
  return true;
}

bool write(File file, Value& value, int mode, char const* func) {
  WriterVisitor writer(file, mode, func);
  writer.setIndent(2);
  if (!value.walk(&writer)) return false;
  return writer.onEnd();
}

bool Visitor::printExStrings = true;

bool CompositeVisitor::onNull() {
  for (auto vis : visitors_) {
    if (!vis->onNull()) {
      return false;
    }
  }
  return true;
}
bool CompositeVisitor::onBoolean(bool val) {
  for (auto vis : visitors_) {
    if (!vis->onBoolean(val)) {
      return false;
    }
  }
  return true;
}
bool CompositeVisitor::onInteger(int val) {
  for (auto vis : visitors_) {
    if (!vis->onInteger(val)) {
      return false;
    }
  }
  return true;
}
bool CompositeVisitor::onNumber(double val) {
  for (auto vis : visitors_) {
    if (!vis->onNumber(val)) {
      return false;
    }
  }
  return true;
}
bool CompositeVisitor::onString(std::string const& val) {
  for (auto vis : visitors_) {
    if (!vis->onString(val)) {
      return false;
    }
  }
  return true;
}
bool CompositeVisitor::onOpenMap() {
  for (auto vis : visitors_) {
    if (!vis->onOpenMap()) {
      return false;
    }
  }
  return true;
}
bool CompositeVisitor::onMapKey(std::string const& key) {
  for (auto vis : visitors_) {
    if (!vis->onMapKey(key)) {
      return false;
    }
  }
  return true;
}
bool CompositeVisitor::onCloseMap() {
  for (auto vis : visitors_) {
    if (!vis->onCloseMap()) {
      return false;
    }
  }
  return true;
}
bool CompositeVisitor::onOpenArray() {
  for (auto vis : visitors_) {
    if (!vis->onOpenArray()) {
      return false;
    }
  }
  return true;
}
bool CompositeVisitor::onCloseArray() {
  for (auto vis : visitors_) {
    if (!vis->onCloseArray()) {
      return false;
    }
  }
  return true;
}
bool CompositeVisitor::onIntegerEx(int val, std::string const& alt) {
  for (auto vis : visitors_) {
    if (!vis->onIntegerEx(val, alt)) {
      return false;
    }
  }
  return true;
}
bool CompositeVisitor::onEnd() {
  for (auto vis : visitors_) {
    if (!vis->onEnd()) {
      return false;
    }
  }
  return true;
}

}
