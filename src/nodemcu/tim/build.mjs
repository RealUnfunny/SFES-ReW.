import * as esbuild from 'esbuild';
import { mkdir, readFile, rm, writeFile } from 'node:fs/promises';
import { gzipSync } from 'node:zlib';

await rm('dist', { recursive: true, force: true });
await mkdir('dist', { recursive: true });

await esbuild.build({
  entryPoints: ['src/main.js'],
  bundle: true,
  minify: true,
  outfile: 'dist/app.js',
  format: 'iife',
  platform: 'browser',
  target: ['es2018'],
});

await esbuild.build({
  entryPoints: ['src/styles.css'],
  bundle: true,
  minify: true,
  outfile: 'dist/styles.css',
});

const [html, js, css] = await Promise.all([
  readFile('index.html', 'utf8'),
  readFile('dist/app.js', 'utf8'),
  readFile('dist/styles.css', 'utf8'),
]);

const singleFileHtml = html
  .replace('<link rel="stylesheet" href="./src/styles.css" />', `<style>${css}</style>`)
  .replace('<script type="module" src="./src/main.js"></script>', `<script>${js}</script>`);

await writeFile('dist/index.html', singleFileHtml, 'utf8');
await writeFile('dist/index.html.gz', gzipSync(singleFileHtml));
