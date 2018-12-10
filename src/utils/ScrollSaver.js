import React from 'react';
import { Route } from 'react-router-dom';

const $history = "$history";

class ScrollSaverComponent extends React.Component {
  onScroll = scrollTop => {
    if (this.scrollTimeout) {
      clearTimeout(this.scrollTimeout);
    }
    this.scrollTimeout = setTimeout(() => {
      delete this.scrollTimeout;
      const history = this.props[$history];
      history.replace(history.location.pathname + history.location.hash, {scrollTop});
    }, 500);
  }

  componentDidMount() {
    const state = this.props[$history].location.state;
    if (state && state.scrollTop != null) {
      this.props.callback(state.scrollTop);
    }
    if (this.props.onRef) {
      this.props.onRef(this);
    }
  }
  componentWillUnmount() {
    if (this.props.onRef) {
      this.props.onRef(null);
    }
  }

  render() {
    return null;
  }
}

export const ScrollSaver = props => <Route render={({history}) => <ScrollSaverComponent {...props} {...{[$history]: history}}/>}/>;
export default ScrollSaver;
