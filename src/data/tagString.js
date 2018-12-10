import React from 'react';

function ColorGroup(arr, color) {
  return key => <span style={{color}} key={key}>{arr}</span>;
}

export default function tagString(value, data) {
  const result = [];
  let color = null;
  let top = result;
  value = value.split("\n").join("|n");
  const regex = /<(\w+),(\w+)(,%)?>|\|([rn])|\|c[0-9a-zA-Z]{2}([0-9a-zA-Z]{6})/gi;
  let prev = 0, match;
  while (match = regex.exec(value)) {
    if (match.index > prev) {
      top.push(value.substring(prev, match.index));
    }
    if (match[1]) {
      const object = data && data.objects.find(obj => obj.id === match[1]);
      const key = match[2].toLowerCase();
      if (object && object.data[key]) {
        top.push(parseFloat(object.data[key]) * (match[3] ? 100 : 1));
      } else {
        top.push(match[0]);
      }
    } else if (match[4] === "n" || match[4] === "N") {
      top.push(<br key={top.length}/>);
    } else if (match[4] === "r" || match[4] === "R" || match[5]) {
      if (color) {
        result.push(color(result.length));
        top = result;
        color = null;
      }
      if (match[5]) {
        top = [];
        color = ColorGroup(top, "#" + match[5]);
      }
    }
    prev = match.index + match[0].length;
  }
  if (prev < value.length) {
    top.push(value.substring(prev));
  }
  if (color) {
    result.push(color(result.length));
  }
  return result;
}
