// Render a comparison layout from unchanged Unreal screenshots.
// Requires Playwright; run from the project root after capturing Original/Lobby.
const fs = require('node:fs');
const path = require('node:path');
const { pathToFileURL } = require('node:url');
const { chromium } = require('playwright');
const root = path.resolve(__dirname, '..');
const out = path.join(root, 'ArtSource/FPSDemo/Samples/UI/FontComparisons');
const variants = [
  ['Original', '原版 · 思源黑体', 'SOURCE HAN SANS SC'],
  ['MiSans', '方案 A · MiSans', 'MISANS'],
  ['HarmonyOS', '方案 B · HarmonyOS Sans SC', 'HARMONYOS SANS SC'],
];
function picture(id, crop) {
  const source = `Previews/${id}/Lobby.png`;
  if (!crop) return `<img class="overview" src="${source}" alt="${id} 大厅实机截图">`;
  const [x, y, w, h] = crop;
  return `<svg viewBox="${x} ${y} ${w} ${h}" width="720" height="${h * 2}" aria-label="截图局部放大"><image href="${source}" width="1920" height="1080"/></svg>`;
}
const html = `<!doctype html><html lang="zh-CN"><meta charset="utf-8"><title>中文字体三版实机对比</title>
<style>
*{box-sizing:border-box}body{margin:0;background:#11191d;color:#edf0ed;font-family:"Microsoft YaHei",sans-serif;width:2256px;padding:32px 24px}
header{display:flex;align-items:baseline;justify-content:space-between;margin-bottom:26px}h1{font-size:34px;margin:0;font-weight:600}header p{font-size:19px;color:#a6b3b8;margin:0}
.grid{display:grid;grid-template-columns:repeat(3,720px);gap:24px}.name{font-size:25px;margin:0 0 7px}.sub{font-size:14px;letter-spacing:2px;color:#8d9fa8;margin-bottom:20px}.card{border-top:4px solid #efd02e;padding-top:16px}
.overview{display:block;width:720px;height:405px}svg{display:block;background:#11191d}h2{font-size:18px;font-weight:400;color:#b2c0c5;margin:22px 0 10px}footer{font-size:18px;color:#9dadb4;margin-top:24px;border-top:1px solid #354248;padding-top:18px}
</style><header><h1>中文字体 · 三版实机对比</h1><p>相同布局 / 字号 / 字重层级 · 英文与数字沿用 Rajdhani</p></header><div class="grid">${variants.map(([id, name, subtitle]) => `<section class="card"><div class="name">${name}</div><div class="sub">${subtitle}</div>${picture(id)}<h2>标题细节 · 200% 放大</h2>${picture(id, [40, 80, 360, 110])}<h2>常规文字 · 200% 放大</h2>${picture(id, [60, 875, 360, 145])}</section>`).join('')}</div><footer>来源：Unreal 实机截图（1920 × 1080） · 局部取自相同坐标，保留字体实际字面与基线差异 · 仅排版与等比缩放</footer></html>`;
async function main() {
  for (const [id] of variants) {
    if (!fs.existsSync(path.join(out, 'Previews', id, 'Lobby.png'))) throw new Error(`Missing ${id} screenshot`);
  }
  const htmlPath = path.join(out, 'ThreeWayComparison.html');
  fs.writeFileSync(htmlPath, html, 'utf8');
  const browser = await chromium.launch({channel:'msedge', headless:true});
  try {
    const page = await browser.newPage({viewport:{width:2256,height:1300},deviceScaleFactor:1});
    await page.goto(pathToFileURL(htmlPath).href);
    await page.evaluate(async () => {
      await document.fonts.ready;
      await Promise.all([...document.images].map(image => image.decode()));
    });
    await page.screenshot({path:path.join(out,'ThreeWayComparison.png'),fullPage:true});
    console.log('PASS: ThreeWayComparison.png rendered from three original screenshots');
  } finally { await browser.close(); }
}
main().catch(error => {console.error(error);process.exitCode=1;});
