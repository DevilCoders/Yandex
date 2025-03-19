declare var io: {
    connect(url: string): Socket;
};

interface Socket {
    on(event: string, callback: (data: any) => void ): void;
    emit(event: string, data: any): void;
}
