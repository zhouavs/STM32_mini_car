const net = require('net');

const PORT = Number(process.env.PORT || process.argv[2] || 9000);
const HOST = process.env.HOST || '0.0.0.0';

const server = net.createServer((socket) => {
  const peer = `${socket.remoteAddress}:${socket.remotePort}`;
  console.log(`[+] conn ${peer}`);
  socket.setNoDelay(true);
  socket.setKeepAlive(true, 15000);

  // 问候
  socket.write(Buffer.from('WELCOME\r\n', 'utf8'));

  // 按 \r\n 解析行
  let buf = Buffer.alloc(0);
  socket.on('data', (chunk) => {
    buf = Buffer.concat([buf, chunk]);
    // 查找 CRLF 作为一帧
    let idx;
    while ((idx = buf.indexOf('\r\n')) !== -1) {
      const frame = buf.subarray(0, idx);        // 不含 CRLF
      buf = buf.subarray(idx + 2);               // 跳过 CRLF
      const line = frame.toString('utf8');

      console.log(`[${peer}] <= ${JSON.stringify(line)}`);

      // 简单协议：PING -> PONG，其他回显
      if (line.trim().toUpperCase() === 'PING') {
        socket.write('PONG\r\n');
      } else if (line.length) {
        socket.write(`ECHO: ${line}\r\n`);
      } else {
        // 空行也回 ACK，便于调试
        socket.write('ACK\r\n');
      }
    }
    // 若数据未以 CRLF 结束，等待下一段
  });

  socket.on('end', () => console.log(`[-] end  ${peer}`));
  socket.on('close', () => console.log(`[-] close ${peer}`));
  socket.on('error', (err) => console.log(`[!] err  ${peer} ${err.message}`));
});

server.listen(PORT, HOST, () => {
  console.log(`TCP server listening on ${HOST}:${PORT}`);
  console.log('提示: 请确保 Windows 防火墙放行该端口。');
});