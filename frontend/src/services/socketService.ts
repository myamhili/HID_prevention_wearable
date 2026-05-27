import { io, Socket } from 'socket.io-client';
import type {
  StateUpdate,
  ActivityUpdate,
  SleepDataUpdate,
  RecordingStatus,
  TrainingStatus,
  StatusUpdate,
  MaxSecondsUpdate,
  LiveDataEvent,
  DeviceState,
} from '../types';

const BACKEND_URL = 'http://127.0.0.1:5000';

export interface SocketServiceCallbacks {
  onStateUpdate?: (data: StateUpdate) => void;
  onActivityUpdate?: (data: ActivityUpdate) => void;
  onSleepDataUpdate?: (data: SleepDataUpdate) => void;
  onRecordingStatus?: (data: RecordingStatus) => void;
  onTrainingStatus?: (data: TrainingStatus) => void;
  onStatusUpdate?: (data: StatusUpdate) => void;
  onMaxSecondsUpdate?: (data: MaxSecondsUpdate) => void;
  onLiveData?: (data: LiveDataEvent) => void;
  onConnect?: () => void;
  onDisconnect?: () => void;
}

class SocketService {
  private socket: Socket | null = null;
  private callbacks: SocketServiceCallbacks = {};

  connect(callbacks: SocketServiceCallbacks) {
    this.callbacks = callbacks;

    this.socket = io(BACKEND_URL, {
      transports: ['websocket', 'polling'],
      reconnection: true,
      reconnectionDelay: 1000,
      reconnectionAttempts: 10,
    });

    // Connection events
    this.socket.on('connect', () => {
      console.log('[SocketService] Connected to backend');
      this.callbacks.onConnect?.();
    });

    this.socket.on('disconnect', () => {
      console.log('[SocketService] Disconnected from backend');
      this.callbacks.onDisconnect?.();
    });

    // Data events
    this.socket.on('state_update', (data: StateUpdate) => {
      this.callbacks.onStateUpdate?.(data);
    });

    this.socket.on('activity_update', (data: ActivityUpdate) => {
      this.callbacks.onActivityUpdate?.(data);
    });

    this.socket.on('sleep_data_update', (data: SleepDataUpdate) => {
      this.callbacks.onSleepDataUpdate?.(data);
    });

    this.socket.on('recording_status', (data: RecordingStatus) => {
      this.callbacks.onRecordingStatus?.(data);
    });

    this.socket.on('training_status', (data: TrainingStatus) => {
      this.callbacks.onTrainingStatus?.(data);
    });

    this.socket.on('status_update', (data: StatusUpdate) => {
      this.callbacks.onStatusUpdate?.(data);
    });

    this.socket.on('max_seconds_update', (data: MaxSecondsUpdate) => {
      this.callbacks.onMaxSecondsUpdate?.(data);
    });

    this.socket.on('live_data', (data: LiveDataEvent) => {
      this.callbacks.onLiveData?.(data);
    });
  }

  disconnect() {
    if (this.socket) {
      this.socket.disconnect();
      this.socket = null;
    }
  }

  // Client-to-server event emitters
  setState(state: DeviceState) {
    this.socket?.emit('set_state', { state });
  }

  setMaxSeconds(maxSeconds: number) {
    this.socket?.emit('set_max_seconds', { maxSeconds });
  }

  startRecording(patientId: string, activity: string) {
    this.socket?.emit('start_recording', { patient_id: patientId, activity });
  }

  stopRecording() {
    this.socket?.emit('stop_recording');
  }

  trainModel(patientId: string) {
    this.socket?.emit('train_model', { patient_id: patientId });
  }

  isConnected(): boolean {
    return this.socket?.connected ?? false;
  }
}

export const socketService = new SocketService();
