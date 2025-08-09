

class InputListener : public RE::BSTEventSink<RE::InputEvent*> {
public:
    // Singleton para garantir que exista apenas uma instância
    static InputListener* GetSingleton() {
        static InputListener singleton;
        return &singleton;
    }

    // A função que processa os eventos de input do jogo
    virtual RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event,
                                                  RE::BSTEventSource<RE::InputEvent*>* a_eventSource) override;

protected:
    InputListener() = default;
    virtual ~InputListener() = default;
    InputListener(const InputListener&) = delete;
    InputListener(InputListener&&) = delete;
    InputListener& operator=(const InputListener&) = delete;
    InputListener& operator=(InputListener&&) = delete;

private:
    // Função para calcular a direção com base nas teclas pressionadas
    void UpdateDirectionalState();

    // Variáveis para rastrear o estado de cada tecla de movimento
    bool w_pressed = false;
    bool a_pressed = false;
    bool s_pressed = false;
    bool d_pressed = false;
};