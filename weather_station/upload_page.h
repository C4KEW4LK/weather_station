#pragma once

static const char UPLOAD_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1"/>
  <title>Upload Files - Weather Station</title>
  <style>
    body {
      font-family: system-ui, -apple-system, sans-serif;
      margin: 20px;
      max-width: 600px;
      background: #f5f5f5;
    }
    .card {
      border: 1px solid #ddd;
      border-radius: 8px;
      padding: 20px;
      margin: 16px 0;
      background: white;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
    }
    h2 {
      margin-top: 0;
      color: #333;
    }
    h3 {
      margin-top: 0;
      font-size: 16px;
      color: #555;
    }
    button {
      padding: 10px 20px;
      border: 1px solid #0275d8;
      border-radius: 4px;
      background: #0275d8;
      color: white;
      cursor: pointer;
      font-size: 14px;
      font-weight: 500;
      margin-top: 8px;
    }
    button:hover {
      background: #025aa5;
    }
    button:disabled {
      background: #ccc;
      border-color: #ccc;
      cursor: not-allowed;
    }
    input[type="file"] {
      margin: 12px 0;
      padding: 8px;
      border: 1px solid #ddd;
      border-radius: 4px;
      width: 100%;
      box-sizing: border-box;
      font-size: 14px;
    }
    .success {
      color: #28a745;
      font-weight: 500;
    }
    .error {
      color: #dc3545;
      font-weight: 500;
    }
    .info {
      color: #666;
      font-size: 14px;
      margin-bottom: 16px;
    }
    .back-link {
      display: inline-block;
      margin-top: 8px;
      color: #0275d8;
      text-decoration: none;
    }
    .back-link:hover {
      text-decoration: underline;
    }
  </style>
</head>
<body>
  <h2>Upload Web Files</h2>
  <div class="info">Upload index.html and/or app.js files to the SD card (/web directory)</div>

  <div class="card">
    <h3>Select Files</h3>
    <div style="margin-bottom: 12px;">
      <label style="display: block; margin-bottom: 4px; font-weight: 500; color: #555;">index.html</label>
      <input type="file" id="htmlFile" accept=".html"/>
    </div>
    <div style="margin-bottom: 12px;">
      <label style="display: block; margin-bottom: 4px; font-weight: 500; color: #555;">app.js</label>
      <input type="file" id="jsFile" accept=".js"/>
    </div>
    <div style="margin-bottom: 12px;">
      <label style="display: block; margin-bottom: 4px; font-weight: 500; color: #555;">Password</label>
      <input type="password" id="password" placeholder="Password" style="width: 200px; padding: 8px; border: 1px solid #ddd; border-radius: 4px; font-size: 14px;"/>
    </div>
    <button onclick="uploadFiles()">Upload</button>
    <div id="status"></div>
  </div>

  <div class="card">
    <a href="/" class="back-link">Back to Dashboard</a>
  </div>

  <script>
    async function uploadFiles() {
      const htmlInput = document.getElementById('htmlFile');
      const jsInput = document.getElementById('jsFile');
      const passwordInput = document.getElementById('password');
      const statusDiv = document.getElementById('status');
      const button = event.target;

      const htmlFile = htmlInput.files && htmlInput.files[0];
      const jsFile = jsInput.files && jsInput.files[0];

      if (!htmlFile && !jsFile) {
        statusDiv.innerHTML = '<p class="error">Please select at least one file</p>';
        return;
      }

      if (!passwordInput.value || passwordInput.value.trim().length === 0) {
        statusDiv.innerHTML = '<p class="error">Password required</p>';
        return;
      }

      const password = passwordInput.value.trim();
      button.disabled = true;
      statusDiv.innerHTML = '<p class="info">Uploading...</p>';

      const uploads = [];
      if (htmlFile) uploads.push({ file: htmlFile, targetName: 'index.html' });
      if (jsFile) uploads.push({ file: jsFile, targetName: 'app.js' });

      let successCount = 0;
      let failCount = 0;
      let messages = [];

      for (const upload of uploads) {
        const formData = new FormData();
        formData.append('file', upload.file);
        formData.append('path', '/web/' + upload.targetName);
        formData.append('pw', password);

        try {
          const response = await fetch('/upload', {
            method: 'POST',
            body: formData
          });

          const result = await response.json();

          if (result.ok) {
            successCount++;
            messages.push('<p class="success">' + upload.targetName + ': Upload successful! (' + result.bytes + ' bytes)</p>');
          } else {
            failCount++;
            messages.push('<p class="error">' + upload.targetName + ': Upload failed - ' + (result.error || 'unknown error') + '</p>');
          }
        } catch (error) {
          failCount++;
          messages.push('<p class="error">' + upload.targetName + ': Upload error - ' + error.message + '</p>');
        }
      }

      statusDiv.innerHTML = messages.join('');
      if (successCount > 0 && failCount === 0) {
        statusDiv.innerHTML += '<p class="success">All files uploaded successfully! Redirecting to dashboard...</p>';
        passwordInput.value = '';
        htmlInput.value = '';
        jsInput.value = '';
        setTimeout(() => {
          window.location.href = '/';
        }, 1500);
      } else {
        button.disabled = false;
      }
    }
  </script>
</body>
</html>
)HTML";
