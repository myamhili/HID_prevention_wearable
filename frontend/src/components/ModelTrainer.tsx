interface ModelTrainerProps {
  patientId: string;
  trainingMessages: string[];
  onTrainModel: (patientId: string) => void;
  isTraining: boolean;
}

export function ModelTrainer({
  patientId,
  trainingMessages,
  onTrainModel,
  isTraining,
}: ModelTrainerProps) {
  const handleTrain = () => {
    if (patientId && !isTraining) {
      onTrainModel(patientId);
    }
  };

  return (
    <div className="card model-trainer">
      <h2>ML Model Trainer</h2>
      <p className="subtitle">Train a new activity recognition model</p>

      <div className="trainer-controls">
        <button
          className="btn btn-primary btn-large"
          onClick={handleTrain}
          disabled={!patientId || isTraining}
        >
          {isTraining ? '⏳ Training...' : '🚀 Train Model'}
        </button>
      </div>

      {trainingMessages.length > 0 && (
        <div className="training-log">
          <h4>Training Progress:</h4>
          <div className="log-container">
            {trainingMessages.map((msg, index) => (
              <div key={index} className="log-entry">
                {msg}
              </div>
            ))}
          </div>
        </div>
      )}

      <div className="info-box">
        <h4>Training Requirements:</h4>
        <p>
          Before training, ensure you have recorded data for both activities:
        </p>
        <ul>
          <li>✓ {patientId}_sitting.csv</li>
          <li>✓ {patientId}_walking.csv</li>
        </ul>
        <p>
          The training process will:
        </p>
        <ol>
          <li>Load both activity datasets</li>
          <li>Compute motion features (velocity/acceleration)</li>
          <li>Create sliding windows from the sensor data</li>
          <li>Normalize the data using StandardScaler</li>
          <li>Train an improved 1D-CNN neural network for 30 epochs</li>
          <li>Save the trained model and scaler for inference</li>
        </ol>
      </div>
    </div>
  );
}
