import { useState, useEffect } from 'react';
import './App.css';
import { socketService } from './services/socketService';
import { ActivityMonitor } from './components/ActivityMonitor';
import { SleepMonitor } from './components/SleepMonitor';
import { DataRecorder } from './components/DataRecorder';
import { ModelTrainer } from './components/ModelTrainer';
import type {
  DeviceState,
  Activity,
  SensorStats,
  StateUpdate,
  ActivityUpdate,
  SleepDataUpdate,
  RecordingStatus,
  TrainingStatus,
  StatusUpdate,
  MaxSecondsUpdate,
} from './types';

/**
 * The main application component. It serves as the central hub for state management,
 * WebSocket communication, and rendering of all child components.
 */
function App() {
  // --- STATE MANAGEMENT ---
  // All application state is managed here and passed down to child components as props.

  // State for the WebSocket connection status.
  const [connected, setConnected] = useState(false);

  // State for the overall device mode ('active' or 'sleeping').
  const [deviceState, setDeviceState] = useState<DeviceState>('sleeping');
  // State for the current patient/user ID.
  const [patientId, setPatientId] = useState('test');
  // State for the maximum inactivity time before alerts.
  const [maxSeconds, setMaxSeconds] = useState(300);

  // State related to activity monitoring.
  const [activity, setActivity] = useState<Activity>('...');
  const [seconds, setSeconds] = useState(300); // Current inactivity countdown.
  const [alert, setAlert] = useState(false);     // True if the inactivity alert is active.
  const [warning, setWarning] = useState('');   // Holds the current warning level text (e.g., "WARN1").

  // State related to sleep monitoring.
  const [tempStats, setTempStats] = useState<SensorStats | null>(null);
  const [sleepDuration, setSleepDuration] = useState(0);

  // State for data recording.
  const [recording, setRecording] = useState(false);
  const [recordingActivity, setRecordingActivity] = useState('');
  const [liveDataCount, setLiveDataCount] = useState(0); // Counter for incoming data points during recording.

  // State for the model training process.
  const [trainingMessages, setTrainingMessages] = useState<string[]>([]);
  const [isTraining, setIsTraining] = useState(false);

  // State for the controlled input field for max seconds.
  const [maxSecondsInput, setMaxSecondsInput] = useState('300');

  // State to manage which primary view is visible ('monitor' or 'training').
  const [activeTab, setActiveTab] = useState<'monitor' | 'training'>('monitor');

  /**
   * Main effect hook for managing the WebSocket connection and event listeners.
   * This runs only once when the component mounts.
   */
  useEffect(() => {
    // Connect to the backend and register all event handlers.
    socketService.connect({
      onConnect: () => {
        console.log('Connected to backend');
        setConnected(true);
      },
      onDisconnect: () => {
        console.log('Disconnected from backend');
        setConnected(false);
      },
      // Handles a full state refresh from the backend, usually on initial connect.
      onStateUpdate: (data: StateUpdate) => {
        console.log('State update:', data);
        setDeviceState(data.state);
        setSeconds(data.seconds);
        if (data.activity) setActivity(data.activity as Activity);
        setPatientId(data.patient);
        setMaxSeconds(data.maxSeconds);
        setMaxSecondsInput(data.maxSeconds.toString());
      },
      // Handles real-time updates for the activity monitor.
      onActivityUpdate: (data: ActivityUpdate) => {
        setActivity(data.activity);
        setSeconds(data.seconds);
        setAlert(data.seconds === 0);
        setWarning(data.warning || '');
      },
      // Handles real-time updates for the sleep monitor.
      onSleepDataUpdate: (data: SleepDataUpdate) => {
        setTempStats(data.temp);
        setSleepDuration(data.sleepDuration);
      },
      // Handles updates on the data recording status.
      onRecordingStatus: (data: RecordingStatus) => {
        setRecording(data.recording);
        if (data.activity) setRecordingActivity(data.activity);
        if (!data.recording) setLiveDataCount(0); // Reset counter when recording stops.
      },
      // Handles real-time status messages from the model training process.
      onTrainingStatus: (data: TrainingStatus) => {
        setTrainingMessages((prev) => [...prev, data.message]);
        if (data.message.includes('complete') || data.message.includes('failed')) {
          setIsTraining(false); // Re-enable training button on completion/failure.
        }
      },
      // Handles general status updates, like inactivity alerts.
      onStatusUpdate: (data: StatusUpdate) => {
        if (data.alert === 'inactive') setAlert(true);
      },
      // Handles confirmation that the max seconds value was updated on the backend.
      onMaxSecondsUpdate: (data: MaxSecondsUpdate) => {
        setMaxSeconds(data.maxSeconds);
        setMaxSecondsInput(data.maxSeconds.toString());
      },
      // A simple event to know live data is flowing during recording.
      onLiveData: () => {
        setLiveDataCount((prev) => prev + 1);
      },
    });

    // Cleanup function: disconnect the socket when the component unmounts.
    return () => {
      socketService.disconnect();
    };
  }, []); // The empty dependency array ensures this effect runs only once.

  // --- EVENT HANDLERS ---
  // These functions are called by user interactions (e.g., button clicks).

  /** Changes the device state between 'active' and 'sleeping'. */
  const handleStateChange = (newState: DeviceState) => {
    socketService.setState(newState);
    setDeviceState(newState); // Optimistically update the UI.
    setAlert(false);
  };

  /** Sends the new max inactivity seconds value to the backend. */
  const handleMaxSecondsChange = () => {
    const value = parseInt(maxSecondsInput, 10);
    if (!isNaN(value) && value > 0) {
      socketService.setMaxSeconds(value);
    }
  };

  /** Starts a data recording session for a given patient and activity. */
  const handleStartRecording = (pid: string, act: string) => {
    socketService.startRecording(pid, act);
  };

  /** Stops the current data recording session. */
  const handleStopRecording = () => {
    socketService.stopRecording();
  };

  /** Initiates the model training process for a given patient. */
  const handleTrainModel = (pid: string) => {
    setTrainingMessages([]); // Clear previous training logs.
    setIsTraining(true);
    socketService.trainModel(pid);
  };

  // --- RENDER LOGIC ---
  return (
    <div className="app">
      <header className="app-header">
        <h1>Delirium Prevention Monitor</h1>
        <div className="connection-status">
          <span className={`status-indicator ${connected ? 'connected' : 'disconnected'}`} />
          {connected ? 'Connected' : 'Disconnected'}
        </div>
      </header>

      {/* Tab navigation to switch between the monitoring and training views. */}
      <div className="tab-navigation">
        <button
          className={`tab-button ${activeTab === 'monitor' ? 'active' : ''}`}
          onClick={() => setActiveTab('monitor')}
        >
          📊 Monitor
        </button>
        <button
          className={`tab-button ${activeTab === 'training' ? 'active' : ''}`}
          onClick={() => setActiveTab('training')}
        >
          🤖 Training & Data
        </button>
      </div>

      <main className="app-main">
        {/* Conditional rendering based on the active tab. */}
        {activeTab === 'monitor' ? (
          <>
            {/* Control Panel for changing device mode and patient settings. */}
            <div className="card control-panel">
              <h2>Control Panel</h2>

              <div className="control-section">
                <h3>Device Mode</h3>
                <div className="button-group">
                  <button
                    className={`btn ${deviceState === 'active' ? 'btn-active' : 'btn-secondary'}`}
                    onClick={() => handleStateChange('active')}
                    disabled={!connected || recording}
                  >
                    🏃 Active Mode
                  </button>
                  <button
                    className={`btn ${deviceState === 'sleeping' ? 'btn-active' : 'btn-secondary'}`}
                    onClick={() => handleStateChange('sleeping')}
                    disabled={!connected || recording}
                  >
                    😴 Sleep Mode
                  </button>
                </div>
              </div>

              <div className="control-section">
                <h3>Patient ID</h3>
                <div className="patient-info">
                  Current: <strong>{patientId}</strong>
                </div>
              </div>

              {/* Only show the 'Max Activity Time' setting when in active mode. */}
              {deviceState === 'active' && (
                <div className="control-section">
                  <h3>Max Activity Time (seconds)</h3>
                  <div className="threshold-control">
                    <input
                      type="number"
                      value={maxSecondsInput}
                      onChange={(e) => setMaxSecondsInput(e.target.value)}
                      min="1"
                      disabled={!connected}
                    />
                    <button
                      className="btn btn-secondary"
                      onClick={handleMaxSecondsChange}
                      disabled={!connected}
                    >
                      Update
                    </button>
                  </div>
                </div>
              )}
            </div>

            {/* Conditionally render either the Activity or Sleep monitor based on device state. */}
            {deviceState === 'active' ? (
              <ActivityMonitor
                activity={activity}
                seconds={seconds}
                maxSeconds={maxSeconds}
                alert={alert}
                warning={warning}
              />
            ) : (
              <SleepMonitor
                temp={tempStats}
                sleepDuration={sleepDuration}
              />
            )}
          </>
        ) : (
          <>
            {/* The Data Recorder component. */}
            <DataRecorder
              recording={recording}
              currentActivity={recordingActivity}
              patientId={patientId}
              onStartRecording={handleStartRecording}
              onStopRecording={handleStopRecording}
              onPatientIdChange={setPatientId}
              liveDataCount={liveDataCount}
            />

            {/* The Model Trainer component. */}

            <ModelTrainer
              patientId={patientId}
              trainingMessages={trainingMessages}
              onTrainModel={handleTrainModel}
              isTraining={isTraining}
            />
          </>
        )}
      </main>

      <footer className="app-footer">
        <p>Delirium Prevention Wearable System</p>
      </footer>
    </div>
  );
}

export default App;
