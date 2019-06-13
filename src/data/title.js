import React from 'react';
import PropTypes from 'prop-types';

const TitleContext = React.createContext(null);

export default class Title extends React.PureComponent {
  static contextType = TitleContext;
  static propTypes = {
    title: PropTypes.string.isRequired,
    combiner: PropTypes.func,
  }
  static defaultProps = {
    combiner: (prev, current) => `${prev} - ${current}`,
  }

  children = []

  componentDidMount() {
    if (this.context) {
      this.context.children.push(this);
      this.context.updateTitle();
    }
  }
  componentDidUpdate() {
    this.updateTitle();
  }
  componentWillUnmount() {
    if (this.context) {
      let index = this.context.children.indexOf(this);
      if (index >= 0) {
        this.context.children.splice(index, 1);
        if (index >= this.context.children.length) {
          this.context.updateTitle();
        }
      }
    }
  }

  updateTitle() {
    let root = this;
    while (root.context) {
      root = root.context;
    }
    document.title = root.getTitle();
  }
  getTitle(prev) {
    let {title, combiner} = this.props;
    if (prev) {
      title = combiner(prev, title);
    }
    if (this.children.length) {
      title = this.children[this.children.length - 1].getTitle(title);
    }
    return title;
  }

  render() {
    const {children} = this.props;
    if (!children) {
      return null;
    }
    return (
      <TitleContext.Provider value={this}>
        {children}
      </TitleContext.Provider>
    );
  }
}
