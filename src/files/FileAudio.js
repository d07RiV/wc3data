import React from 'react';

export default class FileAudioView extends React.PureComponent {
  render() {
    const { audio } = this.props;
    return (
      <div>
        <audio controls controlsList="nodownload" src={audio}/>
      </div>
    );
  }
}
