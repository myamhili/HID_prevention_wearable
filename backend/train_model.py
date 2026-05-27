import pandas as pd
import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import DataLoader, TensorDataset
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
import joblib
import os
from shared_config import WINDOW_SIZE, STEP_SIZE, ACTIVITIES, NUM_CLASSES, parse_full_packet

# --- Global Configuration ---
# Set the computation device to GPU (cuda) if available, otherwise use CPU.
DEVICE = torch.device("cuda" if torch.cuda.is_available() else "cpu")

# --- 1. Model Architecture ---
# This defines the structure of our Neural Network for Human Activity Recognition (HAR).
# It's a 1D Convolutional Neural Network (CNN), which is effective for sequence data like time-series from sensors.
class HARModel(nn.Module):
    def __init__(self, num_classes):
        super(HARModel, self).__init__()
        # The input has 6 channels: raw accelerometer data (X, Y, Z) and jerk (deltaX, deltaY, deltaZ).
        # First convolutional layer: takes 6 channels, outputs 64 feature maps.
        self.conv1 = nn.Conv1d(in_channels=6, out_channels=64, kernel_size=3, padding=1)
        # Batch normalization stabilizes and speeds up training.
        self.bn1 = nn.BatchNorm1d(64)
        # ReLU (Rectified Linear Unit) is a standard activation function.
        self.relu1 = nn.ReLU()
        # Max pooling reduces the dimensionality of the features.
        self.pool1 = nn.MaxPool1d(kernel_size=2)

        # Second convolutional layer: increases feature depth from 64 to 128.
        self.conv2 = nn.Conv1d(in_channels=64, out_channels=128, kernel_size=3, padding=1)
        self.bn2 = nn.BatchNorm1d(128)
        self.relu2 = nn.ReLU()
        self.pool2 = nn.MaxPool1d(kernel_size=2)

        # Adaptive pooling reduces each channel to a single value, making the network robust to different window sizes.
        self.adaptive_pool = nn.AdaptiveAvgPool1d(1)
        # Flatten the output to a 1D vector to feed into the fully connected layers.
        self.flatten = nn.Flatten()

        # Fully connected (Dense) layers for classification.
        self.fc1 = nn.Linear(128, 64)
        self.relu3 = nn.ReLU()
        # Dropout is a regularization technique to prevent overfitting by randomly zeroing some neurons during training.
        self.dropout = nn.Dropout(0.3)
        # The final output layer maps the 64 features to the number of activity classes.
        self.fc2 = nn.Linear(64, num_classes)

    # Defines the forward pass of the data through the network.
    def forward(self, x):
        # The sequence of operations data flows through.
        x = self.pool1(self.relu1(self.bn1(self.conv1(x))))
        x = self.pool2(self.relu2(self.bn2(self.conv2(x))))
        x = self.adaptive_pool(x)
        x = self.flatten(x)
        x = self.dropout(self.relu3(self.fc1(x)))
        x = self.fc2(x)
        return x

# --- 2. Feature Engineering ---
def compute_motion_features(data):
    # Computes jerk (the change in acceleration) to create richer features for the model.
    # The model gets both the raw acceleration and its rate of change.
    # Args: data (numpy array): Raw [X, Y, Z] accelerometer values.
    # Returns: features (numpy array): [X, Y, Z, deltaX, deltaY, deltaZ] values.
    
    # Calculate the difference between consecutive sensor readings (jerk).
    deltas = np.zeros_like(data)
    deltas[1:] = np.diff(data, axis=0)
    # Assume the first sample has the same jerk as the second.
    deltas[0] = deltas[1]

    # Combine the raw data and the computed jerks into a 6-channel array.
    features = np.concatenate([data, deltas], axis=1)
    return features

# --- 3. Main Training Function ---
def train_model(patient_id="test", status_callback=None):
    # This function orchestrates the entire training process from loading data to saving the final model.
    # status_callback is a function (like `print` or a socket emit) to send progress updates.
    if status_callback is None:
        status_callback = print

    status_callback(f"Starting training for patient: {patient_id}")

    all_data, all_labels = [], []
    # Create a mapping from activity name (e.g., 'still') to a numeric label (e.g., 0).
    activity_map = {name: i for i, name in enumerate(ACTIVITIES)}

    # --- Data Loading ---
    # Loop through each activity type to load its corresponding CSV file.
    for activity_name in ACTIVITIES:
        filename = f"{patient_id}_{activity_name}.csv"
        if not os.path.exists(filename):
            status_callback(f"Warning: File not found, skipping: {filename}")
            continue

        activity_label = activity_map[activity_name]
        status_callback(f"Loading '{filename}'...")

        # Parse each line of the CSV to extract sensor data.
        temp_data = []
        with open(filename, 'r') as f:
            for line in f:
                parsed_dict = parse_full_packet(line)
                if parsed_dict and 'X' in parsed_dict:
                    temp_data.append([parsed_dict['X'], parsed_dict['Y'], parsed_dict['Z']])

        if temp_data:
            # --- Feature Computation ---
            # Convert list to numpy array and compute motion features.
            temp_data_np = np.array(temp_data, dtype=np.float32)
            temp_features = compute_motion_features(temp_data_np)

            # Add the processed data and corresponding labels to our main lists.
            for features in temp_features:
                all_data.append(features)
                all_labels.append(activity_label)
            status_callback(f"  -> Loaded {len(temp_data)} samples with motion features")

    if not all_data:
        status_callback(f"Error: No data found for patient '{patient_id}'. Training aborted.")
        return False

    # --- Windowing ---
    # Create overlapping windows of data, which will be the inputs to the CNN.
    status_callback(f"Total samples loaded: {len(all_data)}. Creating sliding windows...")
    X, y = [], []
    for i in range(0, len(all_data) - WINDOW_SIZE, STEP_SIZE):
        window = all_data[i : i + WINDOW_SIZE]
        label = all_labels[i + WINDOW_SIZE - 1] # Label for the window is the activity at the end of it.
        X.append(window)
        y.append(label)

    X = np.array(X, dtype=np.float32)
    y = np.array(y, dtype=np.int64)
    status_callback(f"Created {len(X)} windows of size {WINDOW_SIZE}")

    # --- Data Scaling ---
    # Normalize the data to have a mean of 0 and a standard deviation of 1. This is crucial for training.
    status_callback("Normalizing data and saving scaler...")
    scaler = StandardScaler()
    # Reshape data to 2D to fit the scaler, which works on a sample-by-feature basis.
    X_flat = X.reshape(-1, 6) # 6 features: X, Y, Z, dX, dY, dZ
    X_flat_scaled = scaler.fit_transform(X_flat)
    # Reshape back to the original 3D window format.
    X_scaled = X_flat_scaled.reshape(X.shape)

    # Save the fitted scaler. This is important so we can apply the exact same normalization to live data.
    scaler_filename = f"{patient_id}_scaler.joblib"
    joblib.dump(scaler, scaler_filename)
    status_callback(f"Scaler saved: {scaler_filename}")

    # --- Data Splitting & DataLoader Creation ---
    # Split the dataset into training and validation sets. Stratify ensures both sets have a similar class distribution.
    X_train, X_val, y_train, y_val = train_test_split(
        X_scaled, y, test_size=0.2, random_state=42, stratify=y
    )
    status_callback(f"Training samples: {len(X_train)}, Validation samples: {len(X_val)}")

    # Create PyTorch TensorDatasets. Note the permutation to match Conv1d's expected input shape: (batch, channels, length).
    train_tensor = TensorDataset(torch.from_numpy(X_train).permute(0, 2, 1), torch.from_numpy(y_train))
    val_tensor = TensorDataset(torch.from_numpy(X_val).permute(0, 2, 1), torch.from_numpy(y_val))

    # Create DataLoaders to efficiently feed data to the model in batches.
    train_loader = DataLoader(train_tensor, batch_size=32, shuffle=True)
    val_loader = DataLoader(val_tensor, batch_size=32, shuffle=False)

    # --- Model Initialization and Training ---
    model = HARModel(num_classes=NUM_CLASSES).to(DEVICE)
    # CrossEntropyLoss is standard for multi-class classification.
    criterion = nn.CrossEntropyLoss()
    # Adam is a popular and effective optimization algorithm.
    optimizer = optim.Adam(model.parameters(), lr=0.001)
    # Reduce learning rate on a plateau to fine-tune the model when learning slows down.
    scheduler = optim.lr_scheduler.ReduceLROnPlateau(optimizer, mode='max', factor=0.5, patience=3)
    num_epochs = 30
    best_acc = 0.0

    status_callback(f"Starting model training on {DEVICE} for {num_epochs} epochs...")
    for epoch in range(num_epochs):
        # --- Training Phase ---
        model.train() # Set the model to training mode (enables dropout).
        for inputs, labels in train_loader:
            inputs, labels = inputs.to(DEVICE), labels.to(DEVICE)
            optimizer.zero_grad()    # Clear previous gradients.
            outputs = model(inputs)  # Forward pass.
            loss = criterion(outputs, labels) # Calculate loss.
            loss.backward()          # Backward pass (compute gradients).
            optimizer.step()         # Update model weights.

        # --- Validation Phase ---
        model.eval() # Set the model to evaluation mode (disables dropout).
        correct, total = 0, 0
        with torch.no_grad(): # Disable gradient calculation for efficiency.
            for inputs, labels in val_loader:
                inputs, labels = inputs.to(DEVICE), labels.to(DEVICE)
                outputs = model(inputs)
                _, predicted = torch.max(outputs.data, 1)
                total += labels.size(0)
                correct += (predicted == labels).sum().item()

        acc = 100 * correct / total
        scheduler.step(acc) # Update learning rate based on validation accuracy.

        if acc > best_acc:
            best_acc = acc
            status_callback(f"Epoch {epoch+1}/{num_epochs} - Val Acc: {acc:.2f}% (NEW BEST)")
        else:
            status_callback(f"Epoch {epoch+1}/{num_epochs} - Val Acc: {acc:.2f}%")

    status_callback(f"Best validation accuracy: {best_acc:.2f}%")

    # --- Save the Final Model ---
    model_filename = f"{patient_id}_model.pth"
    torch.save(model.state_dict(), model_filename)
    status_callback(f"Training complete. Model saved: {model_filename}")

    # --- Final Evaluation ---
    # Provide a per-class breakdown of the model's performance on the validation set.
    status_callback("\nFinal Model Evaluation (on validation data):")
    model.eval()
    class_correct = [0] * NUM_CLASSES
    class_total = [0] * NUM_CLASSES
    with torch.no_grad():
        for inputs, labels in val_loader:
            inputs, labels = inputs.to(DEVICE), labels.to(DEVICE)
            outputs = model(inputs)
            _, predicted = torch.max(outputs, 1)
            for label, prediction in zip(labels, predicted):
                if label == prediction:
                    class_correct[label] += 1
                class_total[label] += 1

    for i, activity in enumerate(ACTIVITIES):
        if class_total[i] > 0:
            acc = 100 * class_correct[i] / class_total[i]
            status_callback(f"  - {activity}: {acc:.1f}% accuracy")

    return True

# This block allows the script to be run directly for testing purposes.
if __name__ == "__main__":
    print("Running in standalone training mode for patient: 'test'")
    train_model(patient_id="test")
