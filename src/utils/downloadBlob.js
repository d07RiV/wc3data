export default function downloadBlob(blob, name) {
  const url = URL.createObjectURL(blob);
  const el = document.createElement("a");
  el.setAttribute("href", url);
  el.setAttribute("download", name);
  el.style.display = "none";
  document.body.appendChild(el);
  el.click();
  setTimeout(() => {
    document.body.removeChild(el);
    URL.revokeObjectURL(url);
  });
}

export { downloadBlob };
