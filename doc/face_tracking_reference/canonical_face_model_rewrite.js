const fs = require('fs');

const lines = `${fs.readFileSync('./canonical_face_model.obj')}`.split('\n');

console.log(lines);

const vertices = [];
const verticesTex = [];
const triangles = [];
const newVerticesTex = [];

for (const line of lines) {
  if (line.substring(0, 2) === 'v ') {
    vertices.push(line);
  }
  else if (line.substring(0, 3) === 'vt ') {
    verticesTex.push(line);
  }
  else if (line.substring(0, 2) === 'f ') {
    const args = line.split(/ +/);
    args.shift();

    const newLine = ['f'];
    for (let arg of args) {
      arg = arg.split('/');
      newVerticesTex[arg[0]-1] = verticesTex[arg[1]-1];
      newLine.push(arg[0]);
    }
    triangles.push(newLine.join(' '));
    //verticesTex.push(line);
  }
}

console.log(newVerticesTex);

const out = [];
vertices.forEach(v => {
    out.push(v);
});
newVerticesTex.forEach(v => {
    out.push(v);
});
triangles.forEach(v => {
    out.push(v);
});
fs.writeFileSync('./canonical_face_model_simple.obj', out.join('\n'));