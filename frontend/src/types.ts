// Device state types
export type DeviceState = 'active' | 'sleeping';
export type Activity = 'still' | 'active' | '...';

// Sensor statistics (for sleep mode)
export interface SensorStats {
  avg: number;
  min: number;
  max: number;
  last: number;
}

// State update event from backend
export interface StateUpdate {
  state: DeviceState;
  seconds: number;
  activity?: string;
  patient: string;
  maxSeconds: number;
}

// Activity update event from backend
export interface ActivityUpdate {
  activity: Activity;
  seconds: number;
  warning?: string;
}

// Sleep data update event from backend
export interface SleepDataUpdate {
  temp: SensorStats;
  sleepDuration: number;
}

// Recording status event from backend
export interface RecordingStatus {
  recording: boolean;
  activity?: string;
}

// Training status event from backend
export interface TrainingStatus {
  message: string;
}

// Status update event from backend
export interface StatusUpdate {
  alert: 'inactive';
}

// Max seconds update event from backend
export interface MaxSecondsUpdate {
  maxSeconds: number;
}

// Live data event from backend
export interface LiveDataEvent {
  data: string;
}
