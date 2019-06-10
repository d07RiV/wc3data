import React from 'react';

const Cell = ({type: Type, value, comment}) => {
  comment = comment && comment.replace("\x1b", "\n");
  const cell = value == null || value == "" ? <Type title={comment}>&nbsp;</Type> : <Type title={comment}>{value.toString()}</Type>;
  return cell;
}

export default class FileSlkView extends React.PureComponent {
  render() {
    const { data } = this.props;
    const iotaX = [...Array(data.cols)].map((_, i) => i);
    const iotaY = [...data.rows].map((_, i) => i);
    return (
      <div className="FileSlk">
        <table>
          {data.rows.length >= 1 && <thead>
            <tr>
              {iotaX.map(i => <Cell type="th" key={i} value={data.rows[0][i] || ""} comment={data.comments && data.comments[0] && data.comments[0][i]}/>)}
            </tr>
          </thead>}
          <tbody>
            {iotaY.filter(i => i > 0).map(i => (
              <tr key={i}>
                {iotaX.map(j => <Cell type="td" key={j} value={data.rows[i] ? data.rows[i][j] : undefined}
                  comment={data.comments && data.comments[i] && data.comments[i][j]}/>)}
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    );
  }
}
