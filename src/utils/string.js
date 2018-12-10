import React from 'react';

const makeRegex = (str, middle) => {
  const prefix = (middle && !str.match(/^\s/) ? "" : "\\b") + "(";
  const suffix = ")" + (str.match(/\s$/) ? "\\b" : "");
  str = str.trim().replace(/[-[\]/{}()*+?.\\^$|]/g, "\\$&");
  if (!str) return;
  return new RegExp(prefix + str + suffix, "i");
};
const underlinerFunc = (value, regex) => {
  let found = false;
  return value.split(regex).map((str, index) => {
    if (!found && str.match(regex)) {
      found = true;
      return <u key={index}>{str}</u>;
    } else {
      return str;
    }
  });
};

function replaceReact(text, regex, func, plain) {
  const result = [];
  let prev = 0, match;
  if (!plain) {
    plain = (start, end) => text.substring(start, end);
  }
  const append = val => {
    if (val == null) {
      return;
    } else if (typeof val === "string" && result.length && typeof result[result.length - 1] === "string") {
      result[result.length - 1] += val;
    } else {
      result.push(val);
    }
  }
  while ((match = regex.exec(text))) {
    if (match.index > prev) {
      append(plain(prev, match.index));
    }
    const element = func(match);
    if (React.isValidElement(element)) {
      append(React.cloneElement(element, {key: match.index}));
    } else {
      append(element);
    }
    prev = match.index + match[0].length;
  }
  if (prev < text.length) {
    append(plain(prev, text.length));
  }
  return result;
}

export { makeRegex, underlinerFunc, replaceReact };
