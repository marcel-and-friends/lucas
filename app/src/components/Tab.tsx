import { IonTab } from "@ionic/react";
import type { ReactNode } from "react";

interface TabProps {
  tab: string;
  children?: ReactNode;
}

export default function Tab({ tab, children }: TabProps) {
  return (
    <IonTab tab={tab}>
      <div id={`${tab}-page`} className="h-full w-full">
        {children}
      </div>
    </IonTab>
  );
}
