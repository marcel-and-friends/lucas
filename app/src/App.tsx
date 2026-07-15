import { BleClient } from "@capacitor-community/bluetooth-le";
import {
  IonApp,
  IonLabel,
  IonTab,
  IonTabBar,
  IonTabButton,
  IonTabs,
  isPlatform,
  setupIonicReact,
} from "@ionic/react";
import FullscreenCentered from "@/components/FullscreenCentered";
import Tab from "@/components/Tab";
import { Button } from "@/components/ui/button";
import { Bridge, SERVICE_UUID } from "@/lib/Bridge";
import Heater from "@/pages/Heater";
import BridgeContext from "@/stores/BridgeContext";
import "@ionic/react/css/core.css";
import { Loader2 } from "lucide-react";
import { StrictMode, useEffect, useState } from "react";

setupIonicReact({ rippleEffect: false });

export default function App() {
  const [bluetoothState, setBluetoothState] = useState<BluetoothState>({
    kind: State.Uninitialized,
  });

  useEffect(() => {
    switch (bluetoothState.kind) {
      case State.Uninitialized:
        BleClient.initialize()
          .then(() => {
            console.log("Bluetooth Initialized");
            setBluetoothState({
              kind: State.PrepareSearch,
            });
          })
          .catch(() => console.error("Failed to initialize bluetooth"));
        break;
      case State.PrepareSearch:
        if (!isPlatform("desktop")) {
          setTimeout(() => setBluetoothState({ kind: State.Searching }), 1000);
        }
        break;
      case State.Searching:
        BleClient.requestDevice({
          services: [SERVICE_UUID],
        })
          .then((device) =>
            BleClient.connect(device.deviceId, () => {
              console.warn("Disconnected from device, searching again");
              setBluetoothState({ kind: State.PrepareSearch });
            }).then(() => device),
          )
          .then((device) =>
            setBluetoothState({
              kind: State.Connected,
              bridge: new Bridge(device),
            }),
          )
          .catch((error) => {
            console.error("Error connecting to device: ", error);
            setBluetoothState({ kind: State.PrepareSearch });
          });

        break;
      case State.Connected:
        break;
    }
  }, [bluetoothState]);

  switch (bluetoothState.kind) {
    case State.Uninitialized:
      return (
        <IonApp>
          <FullscreenCentered />
        </IonApp>
      );
    case State.PrepareSearch:
    case State.Searching:
      if (
        bluetoothState.kind === State.PrepareSearch &&
        isPlatform("desktop")
      ) {
        return (
          <IonApp>
            <FullscreenCentered>
              <Button
                onClick={() =>
                  setBluetoothState({
                    kind: State.Searching,
                  })
                }
              >
                Conectar
              </Button>
            </FullscreenCentered>
          </IonApp>
        );
      }
      return (
        <IonApp>
          <FullscreenCentered>
            <Loader2 className="animate-spin" />
          </FullscreenCentered>
        </IonApp>
      );
    case State.Connected:
      return (
        <IonApp>
          <StrictMode>
            <BridgeContext.Provider value={bluetoothState.bridge}>
              <IonTabs>
                <Tab tab="heater">
                  <Heater />
                </Tab>
                <Tab tab="empty">
                  <FullscreenCentered />
                </Tab>

                <IonTabBar slot="bottom">
                  <IonTabButton tab="heater">
                    <IonLabel>Aquecedor</IonLabel>
                  </IonTabButton>
                  <IonTabButton tab="empty">
                    <IonLabel>Vazio</IonLabel>
                  </IonTabButton>
                </IonTabBar>

                <IonTab tab=""></IonTab>
              </IonTabs>
            </BridgeContext.Provider>
          </StrictMode>
        </IonApp>
      );
  }
}

enum State {
  Uninitialized,
  PrepareSearch,
  Searching,
  Connected,
}

interface Uninitialized {
  kind: State.Uninitialized;
}

interface WaitingForSearchTrigger {
  kind: State.PrepareSearch;
}

interface Searching {
  kind: State.Searching;
}

interface Connected {
  kind: State.Connected;
  bridge: Bridge;
}

type BluetoothState =
  | Uninitialized
  | WaitingForSearchTrigger
  | Searching
  | Connected;
