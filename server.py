from flask import Flask, request, render_template_string, send_from_directory
import os
import time

app = Flask(__name__)
UPLOAD_FOLDER = 'uploads'
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER

if not os.path.exists(UPLOAD_FOLDER):
    os.makedirs(UPLOAD_FOLDER)

@app.route('/', methods=['GET'])
def index():
    image_names = os.listdir('./uploads')
    return render_template_string("""
    <h1>Uploaded Images</h1>
    {% for image in images %}
    <img src="{{ url_for('display_image', filename=image) }}" width="500">
    <hr>
    {% endfor %}
    """, images=image_names)

@app.route('/upload', methods=['POST'])
def upload_file():
    image_data = request.data
    if not image_data:
        return 'No image data!', 400
    
    # Generate a unique filename using a timestamp
    filename = f"image_{int(time.time())}.jpg"
    image_path = os.path.join(app.config['UPLOAD_FOLDER'], filename)
    
    with open(image_path, "wb") as f:
        f.write(image_data)

    return 'Image Uploaded!', 200

@app.route('/uploads/<filename>')
def display_image(filename):
    return send_from_directory(app.config['UPLOAD_FOLDER'], filename)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
