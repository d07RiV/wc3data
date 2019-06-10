import React from 'react';

export function withAsync(requests, Component, Loading, Error) {
  const requestNames = Object.keys(requests);
  return class WithAsync extends React.Component {
    static displayName = `WithAsync(${Component.displayName || Component.name || "Component"})`

    constructor(props, context) {
      super(props, context);
      const state = {};
      requestNames.forEach(name => state[name] = {});
      this.state = state;
    }

    setStateChecked(...args) {
      if (!this.unmounted) {
        this.setState(...args);
      }
    }

    updateRequests(props, context) {
      requestNames.forEach(name => {
        const request = (typeof requests[name] === "function" ? requests[name](props, context) : requests[name]);
        if (this.state[name].request !== request) {
          if (request.then) {
            this.setState({[name]: {request}}, () => {
              request.then(result => this.setStateChecked(prevState => {
                if (prevState[name].request === request) return {[name]: {request, result}};
              }), error => this.setStateChecked(prevState => {
                if (prevState[name].request === request) return {[name]: {request, error}};
              }));
            });
          } else {
            this.setState({[name]: {request, result: request}});
          }
        }
      });
    }

    componentDidMount() {
      this.updateRequests(this.props, this.context);
    }
    componentDidUpdate() {
      this.updateRequests(this.props, this.context);
    }

    componentWillUnmount() {
      this.unmounted = true;
    }

    shouldComponentUpdate(nextProps, nextState) {
      return requestNames.some(name => "error" in nextState[name]) ||
             requestNames.every(name => "result" in nextState[name]);
    }

    render() {
      const success = {}, failure = {}, props = {...this.props};
      let loaded = true, errors = false;
      requestNames.forEach(name => {
        delete props[name];
        const data = this.state[name];
        if ("result" in data) {
          success[name] = data.result;
        } else if ("error" in data) {
          failure[name] = data.error;
          errors = true;
        } else {
          loaded = false;
        }
      });
      if (errors) {
        return Error ? <Error {...props} {...failure}/> : null;
      } else if (loaded) {
        return <Component {...props} {...success}/>;
      } else {
        return Loading ? <Loading {...props}/> : null;
      }
    }
  }
}

export function withAsyncLoading(requests, Component, Error) {
  return withAsync(requests, Component, Component, Error);
}

export default withAsync;
