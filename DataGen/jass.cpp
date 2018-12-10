#include "jass.h"
#include "hash.h"
#include "utils/json.h"

namespace jass
{

inline bool _isspace(char c) {
  return c == ' ' || c == '\t';
}
inline bool _isalpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
inline bool _isdigit(char c) {
  return c >= '0' && c <= '9';
}
inline bool _isalnum(char c) {
  return _isalpha(c) || _isdigit(c);
}

struct Token {
  enum Type {
    End, Ident, Hex, Num, Real, Char, String, Other
  };
  Type type = End;
  std::string text;
  size_t spaces = 0;
  int value = 0;

  void setString(char const* str) {
    text.clear();
    text.push_back('"');
    while (*str) {
      switch (*str) {
      case '"':
        text.push_back('\\');
        text.push_back('"');
        break;
      case '\\':
        text.push_back('\\');
        text.push_back('\\');
        break;
      case '\b':
        text.push_back('\\');
        text.push_back('b');
        break;
      case '\f':
        text.push_back('\\');
        text.push_back('f');
        break;
      case '\r':
        if (str[1] != '\n') {
          text.push_back('\n');
        }
        break;
      case '\t':
        text.push_back('\\');
        text.push_back('t');
        break;
      default:
        text.push_back(*str);
      }
      ++str;
    }
    text.push_back('"');
  }
};

struct Line {
  int spaces;
  char const* text;
  size_t text_len;
  char const* suffix;
  size_t suffix_len;
  size_t index;
  size_t outStart;
  size_t outEnd;
};

class Tokenizer {
public:
  Tokenizer(char const* text)
    : text_(text)
  {}
  Tokenizer(Tokenizer const&) = default;

  Tokenizer& operator=(Tokenizer const& t) = default;

  void next(Token& t);

private:
  char const* text_;
  size_t pos_ = 0;
  inline char peek(size_t pos) {
    return text_[pos];
  }
  inline char getc() {
    return peek(++pos_);
  }
  inline char advance(char& chr) {
    char prev = chr;
    chr = peek(++pos_);
    return prev;
  }
};

void Tokenizer::next(Token& t) {
  t.type = Token::End;
  t.text.clear();
  t.value = 0;
  t.spaces = 0;
  while (_isspace(text_[pos_])) {
    ++t.spaces;
    ++pos_;
  }
  char chr = peek(pos_);
  if (chr == '_' || _isalpha(chr)) {
    while (chr == '_' || _isalnum(chr)) {
      t.text.push_back(advance(chr));
    }
    t.type = Token::Ident;
  } else if (chr == '0' && peek(pos_ + 1) == 'x') {
    t.text.push_back(advance(chr));
    t.text.push_back(advance(chr));
    while (_isalnum(chr)) {
      if (chr >= 'a' && chr <= 'z') {
        t.value = t.value * 16 + 10 + (chr - 'a');
      } else if (chr >= 'A' && chr <= 'Z') {
        t.value = t.value * 16 + 10 + (chr - 'A');
      } else {
        t.value = t.value * 16 + (chr - '0');
      }
      t.text.push_back(advance(chr));
    }
    t.type = Token::Hex;
  } else if (_isdigit(chr) || (chr == '.' && _isdigit(peek(pos_ + 1)))) {
    bool dot = false;
    while (_isdigit(chr) || (!dot && chr == '.')) {
      if (chr == '.') {
        dot = true;
      }
      if (!dot) {
        t.value = t.value * 10 + (chr - '0');
      }
      t.text.push_back(advance(chr));
    }
    t.type = dot ? Token::Real : Token::Num;
  } else if (chr == '\'') {
    t.text.push_back(advance(chr));
    while (chr && chr != '\'') {
      if (chr == '\\') {
        t.text.push_back(advance(chr));
      }
      if (chr) {
        t.value = t.value * 256 + chr;
        t.text.push_back(advance(chr));
      }
    }
    if (chr) {
      t.text.push_back(advance(chr));
    }
    t.type = Token::Char;
  } else if (chr == '"') {
    t.text.push_back(advance(chr));
    while (chr && chr != '"') {
      if (chr == '\\') {
        t.text.push_back(advance(chr));
      }
      if (chr) {
        t.text.push_back(advance(chr));
      }
    }
    if (chr) {
      t.text.push_back(advance(chr));
    }
    t.type = Token::String;
  } else if (chr) {
    t.text.push_back(advance(chr));
    t.type = Token::Other;
  }
}

char const* makeId(int x) {
  static char id[] = "'    '";
  if (x < 1090519040) {
    return nullptr;
  }
  for (int i = 0; i < 4; ++i) {
    if (!isalnum(x & 0xFF)) {
      return nullptr;
    }
    id[4 - i] = char(x & 0xFF);
    x >>= 8;
  }
  return id;
}

class TokenWriter {
public:
  TokenWriter(std::vector<uint8>& file, size_t& pos, Options const& opt)
    : file_(file)
    , pos_(pos)
    , opt_(opt)
  {}

  void spaces(size_t sp) {
    if (!sp) {
      return;
    }
    if (pos_ + sp > file_.size()) {
      file_.resize(pos_ + sp);
    }
    memset(file_.data() + pos_, ' ', sp);
    pos_ += sp;
  }
  void string(char const* str, size_t size) {
    if (!size) {
      return;
    }
    if (pos_ + size > file_.size()) {
      file_.resize(pos_ + size);
    }
    memcpy(file_.data() + pos_, str, size);
    pos_ += size;
  }
  void string(std::string const& str) {
    string(str.data(), str.size());
  } 

  void putc(int c) {
    if (pos_ >= file_.size()) {
      file_.resize(pos_ + 1024);
    }
    file_[pos_++] = c;
  }

  void write(Token const& tok) {
    spaces(tok.spaces);
    if (opt_.restoreId) {
      char const* id;
      if (tok.type == Token::Num && (id = makeId(tok.value))) {
        string(id, 6);
      } else {
        string(tok.text);
      }
    }
    else {
      string(tok.text);
    }
  }
private:
  std::vector<uint8>& file_;
  size_t& pos_;
  Options const& opt_;
};

JASSDo::JASSDo(HashArchive& map, Options const& opt_)
  : map_(map)
  , opt(opt_)
{
  sourceFile = map.open("war3map.j");
  if (!sourceFile) {
    sourceFile = map.open("Scripts\\war3map.j");
  }
  if (sourceFile) {
    source = (char*)sourceFile.data();
    sourceEnd = source + sourceFile.size();

    if (sourceEnd - source >= 3 && (uint8) source[0] == 0xEF && (uint8) source[1] == 0xBB && (uint8) source[2] == 0xBF) {
      source += 3;
    }
  }
  if (auto wtsFile = map.open("strings.lib")) {
    wts = StringLib(wtsFile);
  }
  if (auto namesFile = map.open("names.lib")) {
    names = StringLib(namesFile);
  }
}

char const* JASSDo::getline(size_t& length) {
  char const* result = source;
  length = 0;
  while (source < sourceEnd) {
    char chr = *source++;
    if (chr == '\r') {
      if (source < sourceEnd && *source == '\n') {
        ++source;
      }
      return result;
    }
    if (chr == '\n') {
      return result;
    }
    ++length;
  }
  return length ? result : nullptr;
}
size_t JASSDo::numlines() {
  size_t count = 1;
  char const* ptr = source;
  while (ptr < sourceEnd) {
    char chr = *ptr++;
    if (chr == '\r') {
      if (ptr < sourceEnd && *ptr == '\n') {
        ++ptr;
      }
      ++count;
    } else if (chr == '\n') {
      ++count;
    }
  }
  return count;
}

inline uint32 strHash(std::string const& str) {
  uint32 res = 0;
  for (char c : str) {
    res = res * 0x4321 + c;
  }
  return res;
}

inline void addPaddedNumber(std::string& str, uint32 value, uint32 pad) {
  while (pad <= value) {
    pad *= 10;
  }
  while (pad > 1) {
    pad /= 10;
    str.push_back('0' + (value / pad) % 10);
  }
}

class FunctionRenamer {
public:
  using iterator = std::unordered_map<uint32, uint32>::iterator;

  iterator add(std::string const& name) {
    uint32 value = (uint32)data_.size() + 1;
    return data_.emplace(strHash(name), value).first;
  }

  iterator find(std::string const& name) {
    return data_.find(strHash(name));
  }

  iterator end() {
    return data_.end();
  }

  std::string value(iterator const& it) {
    return fmtstring("Func%04u", it->second);
  }

  void value(iterator const& it, std::string& str) {
    str.append("Func", 4);
    addPaddedNumber(str, it->second, 10000);
  }

private:
  std::unordered_map<uint32, uint32> data_;
};

class VariableTypes {
public:
  size_t typeToIndex(std::string const& type) {
    auto it = index_.find(type);
    if (it == index_.end()) {
      it = index_.emplace(type, types_.size()).first;
      types_.push_back(type);
    }
    return it->second;
  }
  std::string const& indexToType(size_t index) {
    return types_[index];
  }
private:
  std::vector<std::string> types_;
  std::unordered_map<std::string, size_t> index_;
};

class VariableRenamer {
public:
  using iterator = std::unordered_map<uint32, std::pair<size_t, uint32>>::iterator;

  VariableRenamer(VariableTypes& types, bool local)
    : types_(types)
    , local_(local)
  {}

  void clear() {
    for (auto& c : count_) {
      c = 0;
    }
    data_.clear();
  }

  iterator add(std::string const& name, std::string const& type) {
    size_t ti = types_.typeToIndex(type);
    if (ti >= count_.size()) {
      count_.resize(ti + 1, 0);
    }
    uint32 value = ++count_[ti];
    return data_.emplace(strHash(name), std::pair<size_t, uint32>(ti, value)).first;
  }

  iterator find(std::string const& name) {
    return data_.find(strHash(name));
  }

  iterator end() {
    return data_.end();
  }

  std::string value(iterator const& it) {
    std::string const& type = types_.indexToType(it->second.first);
    if (local_) {
      return fmtstring("loc_%s%02u", type.c_str(), it->second.second);
    } else {
      return fmtstring("%s%03u", type.c_str(), it->second.second);
    }
  }

  void value(iterator const& it, std::string& str) {
    std::string const& type = types_.indexToType(it->second.first);
    uint32 id = it->second.second;
    if (local_) {
      str.append("loc_", 4);
      str.append(type);
      addPaddedNumber(str, id, 100);
    } else {
      str.append(type);
      addPaddedNumber(str, id, 1000);
    }
  }

private:
  VariableTypes& types_;
  bool local_;
  std::vector<uint32> count_;
  std::unordered_map<uint32, std::pair<size_t, uint32>> data_;
};

MemoryFile JASSDo::process() {
  std::vector<Line> lines;
  lines.resize(numlines());

  size_t lineLength;
  char const* line;

  buffer.resize(sourceFile.size() + 1);
  char* bufferPtr = buffer.data();

  size_t numLines = 0;
  while ((line = getline(lineLength))) {
    size_t spaces = 0;
    while (spaces < lineLength && isspace((unsigned char) line[spaces])) {
      ++spaces;
    }
    bool inStr = false;

    Line& out = lines[numLines];
    out.index = numLines++;
    out.spaces = spaces;
    out.text = bufferPtr;

    size_t start = spaces;
    size_t end = spaces;
    while (inStr || (end < lineLength && (end == lineLength - 1 || line[end] != '/' || line[end + 1] != '/'))) {
      while (end >= lineLength) {
        // multiline string
        memcpy(bufferPtr, line + start, end - start);
        bufferPtr += end - start;
        *bufferPtr++ = '\n';
        start = 0;
        end = 0;
        if (!(line = getline(lineLength))) {
          break;
        }
      }
      if (line[end] == '"') {
        inStr = !inStr;
      }
      if (inStr && line[end] == '\\') {
        ++end;
      }
      if (end < lineLength) {
        ++end;
      }
    }
    memcpy(bufferPtr, line + start, end - start);
    bufferPtr += end - start;
    out.text_len = bufferPtr - out.text;
    *bufferPtr++ = 0;

    out.suffix = bufferPtr;
    if (end < lineLength) {
      memcpy(bufferPtr, line + end, lineLength - end);
      bufferPtr += lineLength - end;
    }
    out.suffix_len = bufferPtr - out.suffix;
  }
  lines.resize(numLines);

  sourceFile.release();

  VariableTypes typeLib;
  FunctionRenamer functions;
  VariableRenamer locals(typeLib, true);
  VariableRenamer globals(typeLib, false);
  std::unordered_map<std::string, std::string> inlined;

  Token tok, tmp;
  tok.text.reserve(256);
  tmp.text.reserve(64);

  if (opt.renameFunctions) {
    for (auto& line : lines) {
      Tokenizer reader(line.text);
      reader.next(tok);
      if (tok.text == "function") {
        reader.next(tok);
        if (tok.type == Token::Ident && tok.text != "main" && tok.text != "config" && tok.text != "InitCustomTeams" && tok.text.size() <= opt.renameFunctions) {
          functions.add(tok.text);
        }
      }
    }
  }

  std::vector<uint8> out(buffer.size() * 2);
  size_t outpos = 0;
  TokenWriter writer(out, outpos, opt);

  bool inGlobals = false;
  int funcLines = 0;
  bool simpleFunc = false;
  std::string funcName;
  int curIndent = 0;

  for (auto& line : lines) {
    enum Type { tNone, tFunction, tLocal, tReturn };
    Tokenizer reader(line.text);
    line.outStart = outpos;

    int pos = 0;
    Type type = tNone;
    bool needNewline = false;
    bool needIndent = true;
    bool isConstant = false;
    bool isFdecl = false;
    int modc = 0;
    int nextIndent = curIndent;
    std::string varType;
    ++funcLines;

    do {
      reader.next(tok);
      if (pos == 0) {
        if (tok.type == Token::Ident) {
          if (tok.text == "globals") {
            inGlobals = true;
          } else if (tok.text == "endglobals") {
            inGlobals = false;
            if (line.index < lines.size() - 1 && opt.insertLines) {
              needNewline = true;
            }
          } else if (tok.text == "function") {
            type = tFunction;
            locals.clear();
          } else if (tok.text == "endfunction") {
            if (line.index < lines.size() - 1 && opt.insertLines) {
              needNewline = true;
            }
            if (funcLines == 2 && simpleFunc) {
              size_t start = lines[line.index - 1].outStart;
              size_t end = lines[line.index - 1].outEnd;
              uint8 const* data = out.data();
              while (start < end && isspace(data[start])) {
                ++start;
              }
              while (start < end && isalpha(data[start])) {
                ++start;
              }
              while (start < end && isspace(data[start])) {
                ++start;
              }
              while (end > start && isspace(data[end - 1])) {
                --end;
              }
              auto& inl = inlined[funcName];
              inl.clear();
              inl.push_back('(');
              inl.append((char*)data + start, end - start);
              inl.push_back(')');
            }
            simpleFunc = false;
          } else if (tok.text == "local") {
            type = tLocal;
          } else if (tok.text == "return") {
            type = tReturn;
          } else if (tok.text == "constant") {
            isConstant = true;
            --pos;
          } else if (inGlobals) {
            varType = tok.text;
          }

          if (tok.text == "globals" || tok.text == "function" || tok.text == "if" || tok.text == "loop") {
            nextIndent += 1;
          } else if (tok.text == "endglobals" || tok.text == "endfunction" || tok.text == "endif" || tok.text == "endloop") {
            curIndent -= 1;
            nextIndent -= 1;
          } else if (tok.text == "else" || tok.text == "elseif") {
            curIndent -= 1;
          }
        }
      } else if (tok.type == Token::Ident) {
        if (tok.text == "takes" && isFdecl) {
          modc = 1;
        } else if (type == tFunction) {
          type = tNone;
          isFdecl = true;
          if (opt.inlineFunctions) {
            Tokenizer r2(reader);
            funcLines = 0;
            simpleFunc = true;
            r2.next(tmp);
            if (tmp.type != Token::Ident || tmp.text != "takes") {
              simpleFunc = false;
            }
            r2.next(tmp);
            if (tmp.type != Token::Ident || tmp.text != "nothing") {
              simpleFunc = false;
            }
            r2.next(tmp);
            if (tmp.type != Token::Ident || tmp.text != "returns") {
              simpleFunc = false;
            }
            r2.next(tmp);
            if (tmp.type != Token::Ident || tmp.text == "nothing") {
              simpleFunc = false;
            }
            if (simpleFunc) {
              auto it = functions.find(tok.text);
              if (it != functions.end()) {
                funcName = it->second;
              } else {
                funcName = tok.text;
              }
            }
          }
        } else if (type == tLocal || modc == 1) {
          if (varType.empty()) {
            if (tok.text != "nothing" && tok.text != "returns") {
              varType = tok.text;
            }
          } else if (varType != "$") {
            if (tok.text == "array") {
              varType.push_back('s');
            } else {
              if (opt.renameLocals && tok.text.size() <= opt.renameLocals) {
                locals.add(tok.text, varType);
              }
              varType = "$";
            }
          }
        } else if (inGlobals && varType.size()) {
          if (tok.text == "array") {
            varType.push_back('s');
          } else {
            if (opt.renameGlobals && tok.text.size() <= opt.renameGlobals) {
              globals.add(tok.text, varType);
            }
            varType.clear();
          }
        }

        FunctionRenamer::iterator fit;
        VariableRenamer::iterator vit;
        if ((vit = locals.find(tok.text)) != locals.end()) {
          tok.text.clear();
          locals.value(vit, tok.text);
        } else if ((fit = functions.find(tok.text)) != functions.end()) {
          tok.text.clear();
          functions.value(fit, tok.text);
        } else if ((vit = globals.find(tok.text)) != globals.end()) {
          tok.text.clear();
          globals.value(vit, tok.text);
        }

        std::unordered_map<std::string, std::string>::iterator nit;
        if (opt.inlineFunctions && (nit = inlined.find(tok.text)) != inlined.end()) {
          bool canInline = true;
          Tokenizer r2(reader);
          r2.next(tmp);
          if (tmp.type != Token::Other || tmp.text != "(") {
            canInline = false;
          }
		      if (canInline) {
			      r2.next(tmp);
			      if (tmp.type != Token::Other || tmp.text != ")") {
			        canInline = false;
            }
          }
          if (canInline) {
            reader = r2;
            tok.text = nit->second;
          }
        }
      }

      if (isFdecl && tok.type == Token::Other && tok.text == ",") {
        modc = 1;
        varType.clear();
      }
      if (opt.restoreInt) {
        if ((tok.type == Token::Char && !makeId(tok.value)) || tok.type == Token::Hex) {
          tok.type = Token::Num;
          tok.text = std::to_string(tok.value);
        }
      }
      if (opt.renameFunctions && tok.type == Token::String) {
        auto it = functions.find(tok.text.substr(1, tok.text.size() - 2));
        if (it != functions.end()) {
          tok.text.clear();
          tok.text.push_back('"');
          functions.value(it, tok.text);
          tok.text.push_back('"');
        }
      }
      if (opt.restoreStrings && tok.type == Token::String && !strncmp(tok.text.c_str(), "\"TRIGSTR_", 9)) {
        int id = 0;
        size_t pos = 9;
        while (pos < tok.text.size() && isdigit((unsigned char)tok.text[pos])) {
          id = id * 10 + (tok.text[pos++] - '0');
        }
        if (pos < tok.text.size() && tok.text[pos] == '"') {
          auto text = wts.get(id);
          if (text) {
            tok.setString(text);
          }
        }
      }
      if (needIndent) {
        writer.spaces(opt.indent ? opt.indent * curIndent : line.spaces);
        needIndent = false;
      }
      if (opt.getObjectName && tok.type == Token::Ident && tok.text == "GetObjectName") {
        bool good = true;
        uint32 id = 0;
        Tokenizer r2(reader);
        Token tmp;
        r2.next(tmp);
        if (tmp.type != Token::Other || tmp.text != "(") {
          good = false;
        }
        if (good) {
          r2.next(tmp);
          if (tmp.type == Token::Num || tmp.type == Token::Char) {
            id = (uint32)tmp.value;
          } else {
            good = false;
          }
        }
        if (good) {
          r2.next(tmp);
          if (tmp.type != Token::Other || tmp.text != ")") {
            good = false;
          }
        }
        char const* name = nullptr;
        if (good) {
          name = names.get(id);
        }
        if (name) {
          reader = r2;
          tok.type = Token::String;
          tok.setString(name);
        }
      }
      writer.write(tok);
      ++pos;
    } while (tok.type != Token::End);

    if (funcLines == 1 && type != tReturn) {
      simpleFunc = false;
    }

    if (line.suffix_len) {
      if (needIndent) {
        writer.spaces(opt.indent ? opt.indent * curIndent : line.spaces);
      }
      writer.string(line.suffix, line.suffix_len);
    }
    curIndent = nextIndent;
    if (needNewline) {
      writer.putc('\n');
    }
    line.outEnd = outpos;
    writer.putc(0);
  }

  out.resize(outpos);
  return MemoryFile(std::move(out));
}

}
