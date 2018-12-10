import React from 'react';

const imageInfo = data => {
  const view = new DataView(data.buffer, data.byteOffset, data.byteLength);
  if (view.getUint32(0, false) === 0x424C5031) { // BLP1
    return {
      format: "BLP",
      width: view.getUint32(12, true),
      height: view.getUint32(16, true),
    };
  } else if (view.getUint32(0, false) === 0x424C5032) { // BLP2
    return {
      format: "BLP v2",
      width: view.getUint32(12, true),
      height: view.getUint32(16, true),
    };
  } else if (view.getUint32(0, false) === 0x44445320) { // DDS
    return {
      format: "DDS",
      width: view.getUint32(16, true),
      height: view.getUint32(12, true),
    };
  } else if (view.getUint32(0, false) === 0x47494638 && (data[4] === 0x37 || data[4] === 0x39) && data[5] === 0x61) { // GIF87a / GIF89a
    const bpp = 1 + 7 & data[10];
    const hdrPos = 13 + 3 * bpp;
    return {
      format: "GIF",
      width: view.getInt16(hdrPos + 5, true),
      height: view.getInt16(hdrPos + 7, true),
    };
  } else if (view.getUint32(0, false) === 0x89504E47 && view.getUint32(4, false) === 0x0D0A1A0A) { // PNG
    return {
      format: "PNG",
      width: view.getUint32(16, false),
      height: view.getUint32(20, false),
    };
  } else if (data[0] === 0xFF && data[1] === 0xD8 && data[2] === 0xFF && data[data.length - 2] === 0xFF && data[data.length - 1] === 0xD9) {
    return {
      format: "JPG",
      width: "?",
      height: "?",
    };
  } else { // TGA ?
    return {
      format: "TGA",
      width: view.getInt16(12, true),
      height: view.getInt16(14, true),
    };
  }
};

export default class FileImageView extends React.PureComponent {
  render() {
    const { data, image } = this.props;
    const info = imageInfo(data);
    return (
      <div>
        <div>{info.width}x{info.height} ({info.format})</div>
        <div>
          <img src={image} alt="Extracted"/>
        </div>
      </div>
    );
  }
}
