import FullscreenCentered from "@/components/FullscreenCentered";
import useFirmwareEvent from "@/hooks/useFirmwareEvent";
import { IonButton } from "@ionic/react";
import { useState } from "react";

export default function Heater() {
  const [temperature, setTemperature] = useState(0.0);

  useFirmwareEvent("temperature_report", (event) => {
    setTemperature(event.celsius);
  });

  return (
    <FullscreenCentered>
      <IonButton>{temperature}</IonButton>
    </FullscreenCentered>
  );
}
