import { useState } from 'react';

interface DataRecorderProps {
  recording: boolean;
  currentActivity: string;
  patientId: string;
  onStartRecording: (patientId: string, activity: string) => void;
  onStopRecording: () => void;
  onPatientIdChange: (patientId: string) => void;
  liveDataCount: number;
}

const ACTIVITIES = ['still', 'active'];

export function DataRecorder({
  recording,
  currentActivity,
  patientId,
  onStartRecording,
  onStopRecording,
  onPatientIdChange,
  liveDataCount,
}: DataRecorderProps) {
  const [selectedActivity, setSelectedActivity] = useState('still');

  const handleStartRecording = () => {
    if (!recording && patientId && selectedActivity) {
      onStartRecording(patientId, selectedActivity);
    }
  };

  return (
    <div className="card data-recorder">
      <h2>Training Data Recorder</h2>
      <p className="subtitle">Record sensor data to train the ML model</p>

      <div className="recorder-controls">
        <div className="form-group">
          <label htmlFor="patient-id">Patient ID:</label>
          <input
            id="patient-id"
            type="text"
            value={patientId}
            onChange={(e) => onPatientIdChange(e.target.value)}
            disabled={recording}
            placeholder="e.g., patient1"
          />
        </div>

        <div className="form-group">
          <label htmlFor="activity-select">Activity to Record:</label>
          <select
            id="activity-select"
            value={selectedActivity}
            onChange={(e) => setSelectedActivity(e.target.value)}
            disabled={recording}
          >
            {ACTIVITIES.map((activity) => (
              <option key={activity} value={activity}>
                {activity.charAt(0).toUpperCase() + activity.slice(1)}
              </option>
            ))}
          </select>
        </div>

        <div className="recording-actions">
          {!recording ? (
            <button
              className="btn btn-primary"
              onClick={handleStartRecording}
              disabled={!patientId}
            >
              ▶️ Start Recording
            </button>
          ) : (
            <button className="btn btn-danger" onClick={onStopRecording}>
              ⏹️ Stop Recording
            </button>
          )}
        </div>

        {recording && (
          <div className="recording-status">
            <div className="recording-indicator">
              <span className="recording-dot"></span>
              <span className="recording-text">
                Recording <strong>{currentActivity}</strong>
              </span>
            </div>
            <div className="sample-count">
              Samples collected: {liveDataCount}
            </div>
          </div>
        )}
      </div>

      <div className="info-box">
        <h4>Instructions:</h4>
        <ol>
          <li>Enter a patient ID (this will be used to save the model)</li>
          <li>Select the activity you want to record</li>
          <li>Click "Start Recording" and perform the activity</li>
          <li>Record for at least 30 seconds to get good training data</li>
          <li>Click "Stop Recording" when done</li>
          <li>Repeat for both activities (still, active)</li>
          <li>Once both activities are recorded, train the model</li>
        </ol>
      </div>
    </div>
  );
}
