let notifyComp = null;
let pendingNotify = null;

export function setNotifyComponent(comp) {
  notifyComp = comp;
  if (comp && pendingNotify) {
    comp.setNotification(pendingNotify.message, pendingNotify.type);
    pendingNotify = null;
  }
}
export function notifyMessage(message, type="info") {
  if (notifyComp) {
    notifyComp.setNotification(message, type);
  } else {
    pendingNotify = {message, type};
  }
}
