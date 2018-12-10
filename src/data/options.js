import React from 'react';

export default class Options extends React.Component {
  static Context = React.createContext({});

  constructor(props) {
    super(props);

    const state = {};
    if (window.localStorage) {
      const value = window.localStorage.getItem("options");
      if (value) {
        try {
          Object.assign(state, JSON.parse(value));
        } catch (e) {
        }
      }
    }
    state.update = (name, value) => this.setState({[name]: value});
    this.state = state;
  }

  componentDidUpdate() {
    if (window.localStorage) {
      const opt = {...this.state};
      delete opt.update;
      window.localStorage.setItem("options", JSON.stringify(opt));
    }
  }

  render() {
    return (
      <Options.Context.Provider value={this.state}>
        {this.props.children}
      </Options.Context.Provider>
    );
  }
}
