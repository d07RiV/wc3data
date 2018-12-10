#pragma once

#include "types.h"
#include "file.h"
#include "common.h"
#include <vector>
#include <map>

namespace json {

class Visitor;

class Value {
public:
  typedef std::map<std::string, Value> Map;
  typedef std::vector<Value> Array;
  enum Type { tUndefined, tNull, tString, tInteger, tNumber, tObject, tArray, tBoolean };

private:
  Type type_;
  union {
    int int_;
    double number_;
    std::string* string_;
    Map* map_;
    Array* array_;
    bool bool_;
  };
public:

  Value(Type type = tUndefined);
  ~Value() {
    clear();
  }

  Value(bool val);
  //Value(int val);
  //Value(unsigned int val);
  Value(sint32 val);
  Value(uint32 val);
  Value(sint64 val);
  Value(uint64 val);
  Value(double val);
  Value(std::string const& val);
  Value(char const* val);
  Value(Value const& val);

  Value& operator=(Value const& rhs);
  Value& setValue(Value const& rhs) {
    return *this = rhs;
  }

  void clear();
  Type type() const {
    return type_;
  }
  Value& setType(Type type);

  // tBoolean
  bool getBoolean() const;
  Value& setBoolean(bool data);

  // tString
  std::string const& getString() const;
  Value& setString(std::string const& data);
  Value& setString(char const* data);

  // tNumber
  bool isInteger() const;
  int getInteger() const;
  double getNumber() const;
  Value& setInteger(int data);
  Value& setNumber(double data);

  // tObject
  Map const& getMap() const;
  bool has(std::string const& name) const;
  bool has(char const* name) const;
  Value const* get(std::string const& name) const;
  Value* get(std::string const& name);
  Value const* get(char const* name) const;
  Value* get(char const* name);
  Value& insert(std::string const& name, Value const& data);
  void remove(std::string const& name);
  Value& insert(char const* name, Value const& data);
  void remove(char const* name);
  Value const& operator[](std::string const& name) const;
  Value& operator[](std::string const& name);
  Value const& operator[](char const* name) const;
  Value& operator[](char const* name);

  bool hasProperty(char const* name, uint8 type) const {
    Value const* prop = get(name);
    return prop && prop->type() == type;
  }

  // tArray
  Array const& getArray() const;
  uint32 length() const;
  Value const* at(uint32 i) const;
  Value* at(uint32 i);
  Value& insert(uint32 i, Value const& data);
  Value& append(Value const& data);
  void remove(uint32 i);
  Value const& operator[](int i) const;
  Value& operator[](int i);
  void resize(size_t size);

  class Iterator {
    Type type_;
    Map::iterator map_;
    Array::iterator array_;
    friend class Value;
    Iterator(Map::iterator map) : type_(tObject), map_(map) {}
    Iterator(Array::iterator array) : type_(tArray), array_(array) {}
  public:
    Iterator() : type_(tUndefined) {}

    Iterator& operator=(Iterator const& it) {
      type_ = it.type_;
      if (type_ == tObject) map_ = it.map_;
      if (type_ == tArray) array_ = it.array_;
      return *this;
    }
    Iterator& operator++() {
      if (type_ == tObject) ++map_;
      if (type_ == tArray) ++array_;
      return *this;
    }
    bool operator==(Iterator const& it) {
      if (type_ != it.type_) return false;
      if (type_ == tObject) return map_ == it.map_;
      if (type_ == tArray) return array_ == it.array_;
      return true;
    }
    bool operator!=(Iterator const& it) {
      if (type_ != it.type_) return true;
      if (type_ == tObject) return map_ != it.map_;
      if (type_ == tArray) return array_ != it.array_;
      return false;
    }

    Value& operator*() const {
      if (type_ == tObject) return map_->second;
      return *array_;
    }
    Value* operator->() const {
      if (type_ == tObject) return &map_->second;
      return &*array_;
    }
    std::string const& key() const {
      return map_->first;
    }
  };

  class ConstIterator {
    Type type_;
    Map::const_iterator map_;
    Array::const_iterator array_;
    friend class Value;
    ConstIterator(Map::const_iterator map) : type_(tObject), map_(map) {}
    ConstIterator(Array::const_iterator array) : type_(tArray), array_(array) {}
  public:
    ConstIterator() : type_(tUndefined) {}

    ConstIterator& operator=(ConstIterator const& it) {
      type_ = it.type_;
      if (type_ == tObject) map_ = it.map_;
      if (type_ == tArray) array_ = it.array_;
      return *this;
    }
    ConstIterator& operator++() {
      if (type_ == tObject) ++map_;
      if (type_ == tArray) ++array_;
      return *this;
    }
    bool operator==(ConstIterator const& it) {
      if (type_ != it.type_) return false;
      if (type_ == tObject) return map_ == it.map_;
      if (type_ == tArray) return array_ == it.array_;
      return true;
    }
    bool operator!=(ConstIterator const& it) {
      if (type_ != it.type_) return true;
      if (type_ == tObject) return map_ != it.map_;
      if (type_ == tArray) return array_ != it.array_;
      return false;
    }

    Value const& operator*() const {
      if (type_ == tObject) return map_->second;
      return *array_;
    }
    Value const* operator->() const {
      if (type_ == tObject) return &map_->second;
      return &*array_;
    }
    std::string const& key() const {
      return map_->first;
    }
  };

  Iterator begin();
  Iterator end();
  ConstIterator begin() const;
  ConstIterator end() const;

  bool walk(Visitor* visitor) const;
};

class Visitor {
public:
  explicit Visitor(bool throwExceptions = false)
    : throw_(throwExceptions)
  {}

  static bool printExStrings;

  virtual ~Visitor() {}
  virtual bool onNull() { return true; }
  virtual bool onBoolean(bool val) { return true; }
  virtual bool onInteger(int val) { return true; }
  virtual bool onNumber(double val) { return true; }
  virtual bool onString(std::string const& val) { return true; }
  virtual bool onOpenMap() { return true; }
  virtual bool onMapKey(std::string const& key) { return true; }
  virtual bool onCloseMap() { return true; }
  virtual bool onOpenArray() { return true; }
  virtual bool onCloseArray() { return true; }
  virtual bool onIntegerEx(int val, std::string const& alt) {
    if (printExStrings) {
      return onString(alt);
    } else {
      return onInteger(val);
    }
  }

  virtual bool onEnd() { return true; }
  virtual void onError(uint32 line, uint32 col, std::string const& reason) {
    if (throw_) throw Exception("JSON error at (%d:%d): %s", line + 1, col + 1, reason.c_str());
  }

  bool keyNull(std::string const& key) {
    return onMapKey(key) && onNull();
  }
  bool keyBoolean(std::string const& key, bool val) {
    return onMapKey(key) && onBoolean(val);
  }
  bool keyInteger(std::string const& key, int val) {
    return onMapKey(key) && onInteger(val);
  }
  bool keyNumber(std::string const& key, double val) {
    return onMapKey(key) && onNumber(val);
  }
  bool keyString(std::string const& key, char const* val) {
    return onMapKey(key) && (val ? onString(val) : onNull());
  }
  bool keyString(std::string const& key, std::string const& val) {
    return onMapKey(key) && onString(val);
  }
private:
  bool throw_;
};

class CompositeVisitor : public Visitor {
public:
  explicit CompositeVisitor(std::vector<Visitor*> visitors = {})
    : visitors_(visitors)
  {}

  void addVisitor(Visitor& visitor) {
    visitors_.push_back(&visitor);
  }

  virtual bool onNull() override;
  virtual bool onBoolean(bool val) override;
  virtual bool onInteger(int val) override;
  virtual bool onNumber(double val) override;
  virtual bool onString(std::string const& val) override;
  virtual bool onOpenMap() override;
  virtual bool onMapKey(std::string const& key) override;
  virtual bool onCloseMap() override;
  virtual bool onOpenArray() override;
  virtual bool onCloseArray() override;
  virtual bool onIntegerEx(int val, std::string const& alt) override;
  virtual bool onEnd() override;

  virtual void onError(uint32 line, uint32 col, std::string const& reason) override {
    for (auto vis : visitors_) {
      vis->onError(line, col, reason);
    }
  }

private:
  std::vector<Visitor*> visitors_;
};

class BuilderVisitor : public Visitor {
public:
  BuilderVisitor(Value& value, bool throwExceptions = false);
  bool onNull() {
    return setValue(Value::tNull);
  }
  bool onBoolean(bool val) {
    return setValue(val);
  }
  bool onInteger(int val) {
    return setValue(val);
  }
  bool onNumber(double val) {
    return setValue(val);
  }
  bool onString(std::string const& val) {
    return setValue(val);
  }
  bool onOpenMap() {
    bool res = openComplexValue(Value::tObject);
    if (res) state_ = sMapKey;
    return res;
  }
  bool onMapKey(std::string const& key) {
    if (state_ != sMapKey) return false;
    key_ = key;
    state_ = sMapValue;
    return true;
  }
  bool onCloseMap() {
    return closeComplexValue();
  }
  bool onOpenArray() {
    bool res = openComplexValue(Value::tArray);
    if (res) state_ = sArrayValue;
    return res;
  }
  bool onCloseArray() {
    return closeComplexValue();
  }
protected:
  Value& value_;
  std::string key_;
  std::vector<Value*> stack_;
  enum {
    sStart,
    sMapValue,
    sMapKey,
    sArrayValue,
    sFinish,
  } state_;

  template<class T>
  bool setValue(T const& value) {
    switch (state_) {
    case sStart:
      value_.setValue(value);
      break;
    case sMapValue:
      stack_.back()->insert(key_, value);
      state_ = sMapKey;
      break;
    case sArrayValue:
      stack_.back()->append(value);
      break;
    default:
      return false;
    }
    return true;
  }

  bool openComplexValue(Value::Type type);
  bool closeComplexValue();
};

enum {mJSON, mJS, mJSCall};

bool parse(File file, Visitor* visitor, int mode = mJSON, std::string* func = nullptr);
bool parse(File file, Value& value, int mode = mJSON, std::string* func = nullptr, bool throwExceptions = false);

class WriterVisitor : public Visitor {
public:
  WriterVisitor(File const& file, int mode = mJSON, char const* func = nullptr);

  void setIndent(std::string indent) {
    indent_ = indent;
  }
  void setIndent(int indent) {
    indent_.assign(indent, ' ');
  }
  void escapeUnicode(bool escape) {
    escape_ = escape;
  }

  bool onNull();
  bool onBoolean(bool val);
  bool onInteger(int val);
  bool onNumber(double val);
  bool onString(std::string const& val);
  bool onOpenMap() {
    openValue('{');
    return true;
  }
  bool onMapKey(std::string const& key);
  bool onCloseMap() {
    closeValue('}');
    return true;
  }
  bool onOpenArray() {
    openValue('[');
    return true;
  }
  bool onCloseArray() {
    closeValue(']');
    return true;
  }
  bool onEnd();

protected:
  File file_;
  int mode_;
  bool escape_;
  bool empty_;
  bool object_;
  std::string indent_;
  std::string curIndent_;
  void onValue();
  void openValue(char chr);
  void closeValue(char chr);
  void writeString(std::string const& str);
};

bool write(File file, Value& value, int mode = mJSON, char const* func = nullptr);

}
