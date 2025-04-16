import FullscreenCentered from "@/components/FullscreenCentered";
import useConnection from "@/hooks/useConnection";
import useFirmwareEvent from "@/hooks/useFirmwareEvent";
import { IonButton, IonIcon } from "@ionic/react";
import { thermometer, trash } from "ionicons/icons";
import { useState } from "react";
import {
  CartesianGrid,
  Legend,
  Line,
  LineChart,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";

const TARGET = 100;

export default function Heater() {
  const connection = useConnection();

  const [clickTimestamp, setClickTimestamp] = useState<number>(0);
  const [temperatures, setTemperatures] = useState<GraphPoint[]>([]);

  useFirmwareEvent("temperature_report", (event) => {
    if (event.celsius >= TARGET) {
      void connection.sendCommand({
        heater_control: {
          stop: {},
        },
      });
    }

    setTemperatures((temperatures) => [
      ...temperatures,
      {
        temperature: event.celsius,
        timestamp: Math.floor(Date.now() / 1000) - clickTimestamp,
      },
    ]);
  });

  return (
    <FullscreenCentered>
      <div>
        <IonButton
          onClick={() => {
            setClickTimestamp(Math.floor(Date.now() / 1000));
            void connection.sendCommand({
              heater_control: {
                start: {
                  target_celsius: 100,
                },
              },
            });
          }}
        >
          <IonIcon icon={thermometer} />
          {TARGET}°C
        </IonButton>
        <IonButton onClick={() => setTemperatures([])}>
          <IonIcon icon={trash} />
        </IonButton>
      </div>
      <ResponsiveContainer width="80%" height="80%">
        <LineChart data={temperatures}>
          <CartesianGrid strokeDasharray="3" />
          <XAxis dataKey="timestamp" />
          <YAxis domain={[0, 110]} />
          <Tooltip animationDuration={100} />
          <Legend />
          <Line
            type="monotone"
            dataKey="temperature"
            stroke="#8884d8"
            strokeWidth="3px"
            activeDot={{ r: 8 }}
          />
        </LineChart>
      </ResponsiveContainer>
    </FullscreenCentered>
  );
}

interface GraphPoint {
  temperature: number;
  timestamp: number;
}
