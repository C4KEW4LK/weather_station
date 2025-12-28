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
  <div class="info">Upload index.html and app.js files to the SD card (/web directory)</div>

  <div class="card">
    <h3>Upload index.html</h3>
    <input type="file" id="htmlFile" accept=".html"/>
    <div style="margin-top: 8px;">
      <input type="password" id="htmlPassword" placeholder="Password" style="width: 200px; padding: 8px; border: 1px solid #ddd; border-radius: 4px; font-size: 14px;"/>
    </div>
    <button onclick="uploadFile('htmlFile', 'index.html', 'htmlPassword')">Upload HTML</button>
    <div id="htmlStatus"></div>
  </div>

  <div class="card">
    <h3>Upload app.js</h3>
    <input type="file" id="jsFile" accept=".js"/>
    <div style="margin-top: 8px;">
      <input type="password" id="jsPassword" placeholder="Password" style="width: 200px; padding: 8px; border: 1px solid #ddd; border-radius: 4px; font-size: 14px;"/>
    </div>
    <button onclick="uploadFile('jsFile', 'app.js', 'jsPassword')">Upload JavaScript</button>
    <div id="jsStatus"></div>
  </div>

  <div class="card">
    <a href="/" class="back-link">Back to Dashboard</a>
  </div>

  <script>
    async function uploadFile(inputId, targetName, passwordId) {
      const input = document.getElementById(inputId);
      const passwordInput = document.getElementById(passwordId);
      const statusDiv = document.getElementById(inputId.replace('File', 'Status'));
      const button = event.target;

      if (!input.files || !input.files[0]) {
        statusDiv.innerHTML = '<p class="error">Please select a file</p>';
        return;
      }

      if (!passwordInput.value || passwordInput.value.trim().length === 0) {
        statusDiv.innerHTML = '<p class="error">Password required</p>';
        return;
      }

      const file = input.files[0];
      const formData = new FormData();
      formData.append('file', file);
      formData.append('path', '/web/' + targetName);
      formData.append('pw', passwordInput.value.trim());

      button.disabled = true;
      statusDiv.innerHTML = '<p class="info">Uploading...</p>';

      try {
        const response = await fetch('/upload', {
          method: 'POST',
          body: formData
        });

        const result = await response.json();

        if (result.ok) {
          statusDiv.innerHTML = '<p class="success">Upload successful! (' + result.bytes + ' bytes) Reload the main page to see changes.</p>';
          passwordInput.value = '';
        } else {
          statusDiv.innerHTML = '<p class="error">Upload failed: ' + (result.error || 'unknown error') + '</p>';
        }
      } catch (error) {
        statusDiv.innerHTML = '<p class="error">Upload error: ' + error.message + '</p>';
      } finally {
        button.disabled = false;
      }
    }
  </script>
</body>
</html>
)HTML";
