import sys
if(len(sys.argv)>1):
    import os
    os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'
    from tensorflow.keras.applications.resnet50 import ResNet50
    from tensorflow.keras.preprocessing import image
    from tensorflow.keras.applications.resnet50 import preprocess_input, decode_predictions
    import numpy as np

    model = ResNet50(weights='imagenet')

    img_path = sys.argv[1]
    img = image.load_img(img_path, target_size=(224, 224))
    x = image.img_to_array(img)
    x = np.expand_dims(x, axis=0)
    x = preprocess_input(x)

    preds = model.predict(x)
    # decode the results into a list of tuples (class, description, probability)
    # (one such list for each sample in the batch)
    predictions = decode_predictions(preds, top=3)[0]
    print(f"{predictions[0][1]};{predictions[1][1]};{predictions[2][1]}")
