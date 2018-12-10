import React from 'react';
import ReactDOM from 'react-dom';
import classNames from 'classnames';
import { List, defaultCellRangeRenderer } from 'react-virtualized';
import { createChainedFunction, replaceReact } from 'utils';

import './TextView.scss';

function makeRegex(str) {
  str = str.replace(/[-[\]/{}()*+?.\\^$|]/g, "\\$&");
  return new RegExp("(" + str + ")", "ig");
}

function getChildIndex(node, child) {
  for (let i = 0; i < node.childNodes.length; ++i) {
    if (node.childNodes[i] === child) {
      return i;
    }
  }
  return -1;
}
function getChildrenLength(node, count) {
  if (count >= node.childNodes.length) return node.textContent.length;
  let length = 0;
  for (let i = 0; i < count; ++i) {
    length += node.childNodes[i].textContent.length;
  }
  return length;
}
function getNodeOffset(node, anchor, anchorOffset) {
  let offset = 0;
  if (anchor.childNodes.length) {
    offset = getChildrenLength(anchor, anchorOffset);
  } else {
    offset = anchorOffset;
  }
  while (anchor && anchor !== node) {
    const parent = anchor.parentElement || anchor.parentNode;
    offset += getChildrenLength(parent, getChildIndex(parent, anchor));
    anchor = parent;
  }
  if (anchor !== node) return 0;
  return offset;
}
function findNodeOffset(node, offset) {
  if (!node.childNodes.length) return {node, offset: Math.min(node.textContent.length, offset)};
  for (let i = 0; i < node.childNodes.length; ++i) {
    const length = node.childNodes[i].textContent.length;
    if (offset < length || i === node.childNodes.length - 1) {
      return findNodeOffset(node.childNodes[i], offset);
    }
    offset -= length;
  }
  return {node, offset: 0};
}

function findMatches(text, regex, current, lineIndex) {
  let match;
  const matches = [];
  while ((match = regex.exec(text))) {
    matches.push({
      index: match.index,
      length: match[0].length,
      className: (current && current.line === lineIndex && current.offset === match.index ? "match-current" : "match")
    });
  }
  return matches;
}
function highlightMatches(text, matches, offset) {
  const result = [];
  let prev = 0;
  matches.forEach(m => {
    const start = Math.max(m.index - offset, 0), end = Math.min(m.index + m.length - offset, text.length);
    if (end <= start) return;
    if (start > prev) result.push(text.substring(prev, start));
    result.push(<span className={m.className} key={m.index}>{text.substring(start, end)}</span>);
    prev = end;
  });
  if (prev < text.length) {
    result.push(text.substring(prev));
  }
  if (result.length === 1) {
    return result[0];
  }
  return result;
}

export default class TextView extends React.Component {
  state = {
    searchText: "",
    searchMatches: [],
  }
  selection = {}

  componentDidMount() {
    document.addEventListener("copy", this.onCopy);
    document.addEventListener("selectionchange", this.onSelect);
  }
  componentWillUnmount() {
    document.removeEventListener("copy", this.onCopy);
    document.removeEventListener("selectionchange", this.onSelect);
  }

  search(text, direction) {
    const searchMatch = this.state.searchMatch;
    let allMatches = [], nextPos = null, found = null, result = null;
    if (text.trim()) {
      if (text === this.state.searchText) {
        allMatches = this.state.searchMatches;
      } else {
        const textLower = text.toLowerCase();
        this.props.lines.forEach((line, index) => {
          let offset = 0, next;
          const lineLower = line.toLowerCase();
          while ((next = lineLower.indexOf(textLower, offset)) !== -1) {
            allMatches.push({line: index, offset: next});
            offset = next + textLower.length;
          }
        });
      }
      let curMatch = -1;
      if (direction !== -1) {
        const origin = (direction === 1 && searchMatch ?
          {line: searchMatch.line, offset: searchMatch.offset + 1} :
          this.selection.searchPos || {line: 0, offset: 0}
        );
        curMatch = allMatches.findIndex(m => m.line > origin.line || (m.line === origin.line && m.offset >= origin.offset));
        if (curMatch < 0 && allMatches.length) curMatch = 0;
      } else {
        const origin = (searchMatch ? {line: searchMatch.line, offset: searchMatch.offset - 1} :
          this.selection.searchPos || {line: 0, offset: 0}
        );
        allMatches.forEach((m, index) => {
          if (m.line > origin.line || (m.line === origin.line && m.offset > origin.offset)) return;
          curMatch = index;
        });
        if (curMatch < 0 && allMatches.length) curMatch = allMatches.length - 1;
      }
      nextPos = allMatches[curMatch];
      found = nextPos && nextPos.line;
      result = {pos: curMatch, count: allMatches.length};
    }
    if (this._list) {
      this._list.forceUpdateGrid();
    }
    this.setState({
      searchText: text,
      searchMatch: nextPos,
      searchRegex: text && text.trim() ? makeRegex(text) : null,
      searchMatches: allMatches,
    }, () => {
      if (found && this._list) this._list.scrollToRow(found);      
    });
    return result;
  }

  nodeToPos2(node, offset) {
    return this.nodeToPos({node, offset});
  }
  nodeToPos(data) {
    if (!data || !this._node) return;
    const {node, offset} = data;
    if (!this._node.contains(node)) return;
    if (node === this._firstLine) {
      const lines = this._node.querySelectorAll(".line");
      if (!lines.length) return;
      const firstLine = parseInt(lines[0].getAttribute("data-line-number"), 10) - 1;
      return {line: firstLine, offset: 0};
    }
    if (node === this._lastLine) {
      const lines = this._node.querySelectorAll(".line");
      if (!lines.length) return;
      const lastLine = parseInt(lines[lines.length - 1].getAttribute("data-line-number"), 10) - 1;
      return {line: lastLine, offset: lines[lines.length - 1].parentElement.textContent.length - 1};
    }
    let line = node;
    while (line && line !== this._node && (!line.classList || !line.classList.contains("TextLine"))) {
      line = line.parentElement || line.parentNode;
    }
    if (!line || !line.classList || !line.classList.contains("TextLine")) return;
    const lineNode = line.querySelector(".line");
    if (!lineNode) return;
    const lineIndex = parseInt(lineNode.getAttribute("data-line-number"), 10);
    return {line: lineIndex - 1, offset: getNodeOffset(line, node, offset)};
  }
  posToNode2(line, offset) {
    return this.posToNode({line, offset});
  }
  posToNode(data) {
    if (!data || !this._node) return;
    const {line, offset} = data;
    let lineNode = this._node.querySelector(`.line[data-line-number="${line + 1}"]`);
    if (!lineNode) {
      const lines = this._node.querySelectorAll(".line");
      if (!lines.length) return;
      const firstLine = parseInt(lines[0].getAttribute("data-line-number"), 10) - 1;
      if (line < firstLine && this._firstLine) {
        return {node: this._firstLine, offset: 0};
      }
      const lastLine = parseInt(lines[lines.length - 1].getAttribute("data-line-number"), 10) - 1;
      if (line > lastLine && this._lastLine) {
        return {node: this._lastLine, offset: 0};
      }
      return;
    }

    lineNode = lineNode.parentElement;
    return findNodeOffset(lineNode, offset);
  }
  
  onCopy = () => {
    if (!this._node) return;
    const selection = window.getSelection();
    const tempNodes = [];
    const ts = this.selection;
    const anchorPos = this.nodeToPos2(selection.anchorNode, selection.anchorOffset);
    const focusPos = this.nodeToPos2(selection.focusNode, selection.focusOffset);
    if (ts.anchorPos && ts.focusPos && anchorPos && focusPos) {
      const tsBegin = (ts.anchorPos.line < ts.focusPos.line ? ts.anchorPos : ts.focusPos);
      const tsEnd = (ts.anchorPos.line < ts.focusPos.line ? ts.focusPos : ts.anchorPos);
      const csBeginNode = (anchorPos.line < focusPos.line ? selection.anchorNode : selection.focusNode);
      const csBegin = (anchorPos.line < focusPos.line ? anchorPos : focusPos);
      const csEndNode = (anchorPos.line < focusPos.line ? selection.focusNode : selection.anchorNode);
      const csEnd = (anchorPos.line < focusPos.line ? focusPos : anchorPos);
      const raw = this.props.lines;
      if (tsBegin.line < csBegin.line && csBeginNode.nodeType === 1) {
        const lines = [];
        for (let i = tsBegin.line; i <= csBegin.line; ++i) {
          let line = raw[i];
          if (!line) continue;
          if (i === tsBegin.line) line = line.substr(tsBegin.offset);
          else if (i === csBegin.line) line = line.substr(0, csBegin.offset);
          lines.push(line);
        }
        const prepend = lines.join("\n");
        if (prepend.length) {
          const tempNode = document.createElement("span");
          tempNode.style.position = "absolute";
          tempNode.style.left = "-10000px";
          tempNode.style.top = "-10000px";
          tempNode.style.fontSize = "0";
          tempNode.style.whiteSpace = "pre";
          tempNode.innerText = prepend;
          tempNodes.push(tempNode);
          csBeginNode.appendChild(tempNode);
        }
      }
      if (tsEnd.line > csEnd.line && csEndNode.nodeType === 1) {
        const lines = [];
        for (let i = csEnd.line + 1; i <= tsEnd.line; ++i) {
          let line = raw[i];
          if (!line) continue;
          if (i === csEnd.line) line = line.substr(csEnd.offset);
          else if (i === tsEnd.line) line = line.substr(0, tsEnd.offset);
          lines.push(line);
        }
        const append = lines.join("\n");
        if (append.length) {
          const tempNode = document.createElement("span");
          tempNode.style.position = "absolute";
          tempNode.style.left = "-10000px";
          tempNode.style.top = "-10000px";
          tempNode.style.fontSize = "0";
          tempNode.style.whiteSpace = "pre";
          tempNode.innerText = append;
          tempNodes.push(tempNode);
          csEndNode.parentElement.insertBefore(tempNode, csEndNode);
        }
      }
    }
    if (tempNodes.length) {
      window.setTimeout(() => {
        tempNodes.forEach(node => node.parentNode.removeChild(node));
      }, 100);
    }
  }
  
  onSelect = () => {
    if (this.suppressSelect || !this.selection || !this._node) return;
    const selection = window.getSelection();
    const ts = this.selection;

    const badNode = node => {
      let line = node;
      while (line && line !== this._node && line !== this._firstLine &&
          line !== this._lastLine && (!line.classList || !line.classList.contains("TextLine"))) {
        line = line.parentElement || line.parentNode;
      }
      if (line === this._firstLine || line === this._lastLine) return false;
      return !line.classList || !line.classList.contains("TextLine");
    };

    if (!selection.anchorNode || !this._node.contains(selection.anchorNode)) {
      ts.anchorPos = null;
    } else if (!badNode(selection.anchorNode)) {
      const p2n = this.posToNode(ts.anchorPos);
      const tsPos = this.nodeToPos(p2n);
      const csPos = this.nodeToPos2(selection.anchorNode, selection.anchorOffset);
      if (tsPos && csPos && tsPos.line === csPos.line && tsPos.offset === csPos.offset) {
        selection.setBaseAndExtent(p2n.node, p2n.offset, selection.focusNode, selection.focusOffset);
      } else {
        ts.anchorPos = csPos;
        ts.searchPos = csPos;
      }
    }

    if (!selection.focusNode || !this._node.contains(selection.focusNode)) {
      ts.focusPos = null;
    } else if (!badNode(selection.focusNode)) {
      const tsPos = this.nodeToPos(this.posToNode(ts.focusPos));
      const csPos = this.nodeToPos2(selection.focusNode, selection.focusOffset);
      if (tsPos && csPos && tsPos.line === csPos.line && tsPos.offset === csPos.offset) {
        // do nothing
      } else {
        ts.focusPos = csPos;
      }
    }
  }
  selectRange(fromPos, toPos) {
    const selection = window.getSelection();
    const from = this.posToNode(fromPos);
    const to = this.posToNode(toPos);
    selection.setBaseAndExtent(
      from ? from.node : selection.anchorNode,
      from ? from.offset : selection.anchorOffset,
      to ? to.node : selection.focusNode,
      to ? to.offset : selection.focusOffset
    );
    this.selection.anchorPos = fromPos;
    this.selection.searchPos = fromPos;
    this.selection.focusPos = toPos;
  }
  onScroll = ({scrollTop}) => {
    const ts = this.selection;
    if (!ts || !ts.anchorPos || !ts.focusPos) return;
    this.suppressSelect = true;
    this.selectRange(ts.anchorPos, ts.focusPos);
    if (this.unsuppress) clearTimeout(this.unsuppress);
    this.unsuppress = setTimeout(() => {
      this.suppressSelect = false;
      delete this.unsuppress;
    }, 100);
  }

  cellRangeRenderer = (props) => {
    const children = defaultCellRangeRenderer(props);
    children.unshift(<div key="first-line" className="fake-line" ref={node => this._firstLine = node}><span/></div>);
    children.push(<div key="last-line" className="fake-line" ref={node => this._lastLine = node}><span/></div>);
    return children;
  }

  rowRenderer = ({index, key, style}) => {
    const { highlightExpr, highlightFunc } = this.props;
    if (this.props.padding) {
      if (index === 0 || index > this.props.lines.length) {
        return null;
      } else {
        index -= 1;
      }
    }
    const search = this.state.searchRegex;
    let line = this.props.lines[index];
    const matches = search && findMatches(line, search, this.state.searchMatch, index);
    if (highlightExpr && highlightFunc) {
      line = replaceReact(line, highlightExpr, match => {
        let str = match[0];
        if (matches) {
          str = highlightMatches(match[0], matches, match.index);
        }
        return highlightFunc(str, match);
      }, (start, end) => {
        let str = line.substring(start, end);
        if (matches) {
          str = highlightMatches(str, matches, start);
        }
        return str;
      });
    } else if (matches) {
      line = highlightMatches(line, matches, 0);
    }
    return (
      <div className="TextLine" key={key} style={style}>
        <span className="line" data-line-number={index + 1}/>
        {line}{"\n"}
        <span className="eol"/>
      </div>
    );
  }

  rowMeasure = ({index}) => {
    if (this.props.padding) {
      if (index === 0 || index > this.props.lines.length) {
        return this.props.padding;
      } else {
        index -= 1;
      }
    }
    return this.props.heights[index];
  }

  componentDidUpdate() {
    if (this._lines !== this.props.lines) {
      this._list.recomputeRowHeights();
      this._lines = this.props.lines;
      this.selection = {};
    }
  }

  setScroll = scrollTop => {
    if (this._list) {
      this._list.scrollToPosition(scrollTop);
    } else {
      this._scrollTop = scrollTop;
    }
  }
  setList = e => {
    this._list = e;
    this._node = ReactDOM.findDOMNode(e);
    if (this._scrollTop != null) {
      this._list.scrollToPosition(this._scrollTop);
      delete this._scrollTop;
    }
  }

  render() {
    const {
      className,
      onScroll,
      heights,
      lines,
      highlightExpr,
      highlightFunc,
      padding,
      ...props
    } = this.props;
    let count = lines.length + (padding ? 2 : 0);
    if (this._heights !== heights) {
      this._heights = heights;
      this._avgHeight = heights.reduce((sum, h) => sum + h, (padding || 0) * 2) / count;
    }
    return <List
      className={classNames(className, "TextView")}
      ref={this.setList}
      rowCount={count}
      estimatedRowSize={this._avgHeight}
      onScroll={createChainedFunction(this.onScroll, onScroll)}
      cellRangeRenderer={this.cellRangeRenderer}
      rowHeight={this.rowMeasure}
      rowRenderer={this.rowRenderer}
      {...props}
    />;
  }
}
