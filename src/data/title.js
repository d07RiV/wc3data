import React from 'react';

const TitleContext = React.createContext(null);

class TitleRoot extends React.Component {
  titles = new Map();
  current = null;

  componentDidMount() {
    this.prefix = document.title;
  }

  setTitle(key, value) {
    this.titles.set(key, value);
    if (!this.current) {
      this.current = key;
    }
    if (key === this.current) {
      document.title = `${this.prefix} - ${value}`;
    }
  }
  unsetTitle(key) {
    this.titles.delete(key);
    if (this.current === key) {
      if (this.titles.size) {
        const val = this.titles.entries().next().value;
        this.current = val[0];
        document.title = `${this.prefix} - ${val[1]}`;
      } else {
        this.current = null;
        document.title = this.prefix;
      }
    }
  }

  render() {
    const {children} = this.props;
    return (
      <TitleContext.Provider value={this}>
        {children}
      </TitleContext.Provider>
    );
  }
}

export default class Title extends React.Component {
  static contextType = TitleContext;

  static Root = TitleRoot;

  componentDidMount() {
    this.context.setTitle(this, this.props.title);
  }
  componentDidUpdate() {
    this.context.setTitle(this, this.props.title);
  }
  componentWillUnmount() {
    this.context.unsetTitle(this);
  }

  render() {
    return null;
  }
}
