export const healthRoutes = async (app) => {
    app.get('/health', async () => {
        return { status: 'ok' };
    });
};
