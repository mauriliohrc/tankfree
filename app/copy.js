// Copy the web editor (manager/) into renderer/ so electron-builder can pack it.
// manager/ stays the single source of truth; renderer/ is generated (gitignored).
const fs = require("fs");
const path = require("path");

const src = path.join(__dirname, "..", "manager");
const dst = path.join(__dirname, "renderer");

function copyDir(s, d) {
  fs.mkdirSync(d, { recursive: true });
  for (const name of fs.readdirSync(s)) {
    if (name === "README.md") continue;
    const sp = path.join(s, name), dp = path.join(d, name);
    const st = fs.statSync(sp);
    if (st.isDirectory()) copyDir(sp, dp);
    else fs.copyFileSync(sp, dp);
  }
}

fs.rmSync(dst, { recursive: true, force: true });
copyDir(src, dst);
console.log("copied manager/ -> app/renderer/");
