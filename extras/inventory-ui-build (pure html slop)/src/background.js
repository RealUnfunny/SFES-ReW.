import { dom } from './dom.js';

const ctx = dom.canvas.getContext('2d');
const text = 'THATINVENTORYMANAGER';
let offset = 0;

function resizeCanvas() {
  dom.canvas.width = window.innerWidth;
  dom.canvas.height = window.innerHeight;
}

function draw() {
  ctx.clearRect(0, 0, dom.canvas.width, dom.canvas.height);
  offset += 0.5;
  ctx.font = '32px monospace';
  ctx.fillStyle = 'rgba(255, 255, 255, 0.08)';

  const xPos = dom.canvas.width * 0.15;
  const spacing = 34;

  for (let y = -spacing; y < dom.canvas.height + spacing; y += spacing) {
    const yPos = (y + offset) % (dom.canvas.height + spacing);
    const index = Math.floor(Math.abs(y) / spacing) % text.length;
    ctx.fillText(text[index], xPos, yPos);
  }

  requestAnimationFrame(draw);
}

export function initBackground() {
  window.addEventListener('resize', resizeCanvas);
  resizeCanvas();
  draw();
}
